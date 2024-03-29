
#include "StimWangCtrl.hh"
#include "DelayGeyer.hh"
#include "coman_properties.hh"
#include "user_realtime.h" // TODO: For plotting 
#include <stdio.h>

#define TA_EXTRA_K 4.0
#define TA_EXTRA_THRESHOLD -0.1
#define EXTRA_TA

inline double pos(double x) { return (x > 0.0) ?  x : 0.0; }
inline double neg(double x) { return (x < 0.0) ? -x : 0.0; }

// extern global variable -> to remove at the end !!!
extern double speed_fwd_global;

/*! \brief constructor
 * 
 * \param[in] inputs controller inputs
 * \param[in] ws walk states
 * \param[in] fwd_kin forward kinematics
 * \param[in] parts body parts
 * \param[in] options controller options
 */
StimWangCtrl::StimWangCtrl(CtrlInputs *inputs, WalkStates *ws, ForwardKinematics *fwd_kin, BodyPart **parts, CtrlOptions *options): StimulationCtrl(inputs, ws, fwd_kin, parts, options)
{
	opti_init = new OptiInit();

	sw_st = static_cast<SwingStanceState*>(ws->get_state(SWING_STANCE_STATE));
	tr_st = static_cast<TrailingState*>(ws->get_state(TRAILING_STATE));

	delay_manager = new DelayGeyer();

	flag_3D = options->is_flag_3D();
	ctrl_two_parts = options->is_ctrl_two_parts();
	Qq_match_wang = options->is_Qq_match_wang();
	inital_pos = options->is_initial_pos(); 

	flag_part1 = 0;
	flag_part2 = 0;

	// Oscillator gains 
	k_HFLrun1 = 0.0;
	k_HFLrun2 = 0.0;
	k_HAMrun = 0.0;

	if (flag_3D)
	{
		std::cout << "Error: 3D walking not implemented for Wang controller" << std::endl;
		exit(EXIT_FAILURE);
	}

	for (int i=0; i<NB_LEGS; i++)
	{
		pk[i] = parts[i]->get_articulations(PITCH_KNEE_ART);
		ph[i] = parts[i]->get_articulations(PITCH_HIP_ART);
		
		sol[i] = parts[i]->get_muscle(SOL_MUSCLE);
		ta[i]  = parts[i]->get_muscle(TA_MUSCLE);
		vas[i] = parts[i]->get_muscle(VAS_MUSCLE);
		ham[i] = parts[i]->get_muscle(HAM_MUSCLE);
		hfl[i] = parts[i]->get_muscle(HFL_MUSCLE);
		gas[i] = parts[i]->get_muscle(GAS_MUSCLE);
		glu[i] = parts[i]->get_muscle(GLU_MUSCLE);

		// angles (delay)
		phi_k[i]  = 0.0;
		phip_k[i] = 0.0;
		phi_h[i]  = 0.0;
		phip_h[i] = 0.0;

		// l.ce (delay)
		lce_ta[i]  = 0.0;
		lce_ham[i] = 0.0;
		lce_hfl[i] = 0.0;

		// Fm (delay)
		F_sol[i] = 0.0;
		F_vas[i] = 0.0;
		F_gas[i] = 0.0;
		F_ham[i] = 0.0;
		F_glu[i] = 0.0;
	}

	// torso (delay)
	theta_torso = 0.0;
	omega_torso = 0.0;
	theta_toro_sw0 = 0.0;

	// fixed parameters
	F_max_sol = sol[R_ID]->get_Fmax();
	F_max_vas = vas[R_ID]->get_Fmax();
	F_max_gas = gas[R_ID]->get_Fmax();
	F_max_ham = ham[R_ID]->get_Fmax();
	F_max_glu = glu[R_ID]->get_Fmax();
	l_opt_ta  = ta[R_ID]->get_lopt();
	l_opt_ham = ham[R_ID]->get_lopt();
	l_opt_hfl = hfl[R_ID]->get_lopt();

	// set opti default parameters
	set_opti_defaults();    

	// switch controller nb of steps
	nb_strikes_switch = 6;

	// for optimization
	inputs->get_opti_inputs()->set_stim_ctrl(this);

	first_swing[R_ID] = 1;
	first_swing[L_ID] = 1;

	// TODO:
	first_stance[R_ID] = 1;
	first_stance[L_ID] = 1;

	// added ghost oscillator object
	this->ghost_osc = new MatsuokaSixN(6, inputs->get_t(), ws, inputs, options); 

	// add cpg control time threshold parameter
	cpg_ctrl_thresh_t = 0.0;

	// add flag indicating when cpg control active
	cpg_ctrl_active = 0;
}

/*! \brief Set StimWang parameters to defaults
 */
void StimWangCtrl::set_opti_defaults()
{
	// ---- opti parameters ----
	// pre-stimulations
	S0_sol_st = 0.04144467;
	S0_ta_st  = 0.03442493;
	S0_gas_st = 0.01144743;
	S0_vas_st = 0.27724696;
	S0_ham_st = 0.12211856;
	S0_rf_st  = 0.03856261;
	S0_glu_st = 0.19368160;
	S0_hfl_st = 0.08483506;
	S0_sol_sw = 0.03201217;
	S0_ta_sw  = .02341347 ;
	S0_gas_sw = 0.05998866;
	S0_vas_sw = 0.07705747;
	S0_ham_sw = 0.08176211;
	S0_rf_sw  = .29242350 ;
	S0_glu_sw = 0.01436186;
	S0_hfl_sw = 0.03742033;
	// gains of positive force feedback laws
	G_sol = 3.20496133;
	G_sol_ta = 3.15520137;
	G_gas = 9.63917261;
	G_vas = 1.64607262;
	G_ham = 1.11480642;
	G_glu = 0.07998873;
	// gains of positive length feedback laws
	G_ta_sw = 2.71763514;
	G_ta_st = 3.70742092;
	G_hfl = 0.82179641;
	G_ham_hfl = 3.85110420;
	// offsets of positive length feedback laws
	l_off_ta_sw = 0.63260836;
	l_off_ta_st = 0.50250814;
	l_off_ham_hfl = 0.00736140;
	l_off_hfl = 0.53396689 ;
	// stance phase PD-control parameters
	K_ham = 4.06202134 ; //1.0;
	K_glu = 13.99331892; //1.0;
	K_hfl = 5.05426992; //1.0;
	D_ham = 0.30785626; //0.2;
	D_glu = 0.49261734; //0.2;
	D_hfl = 0.12076677; //0.2;
	theta_ref = 0.06586455; //0.105;
	// swing initiation parameters
	si_vas = 0.42227731;
	si_rf = 0.32872840 ;
	si_glu = 0.65210655;
	si_hfl = 0.59414450;
	// stance preparation muscle PD-control parameters
	K_sp_vas = 0.13819734;
	K_sp_glu = 2.95331736;
	K_sp_hfl = 0.91644294;
	D_sp_vas = 0.03033318;
	D_sp_glu = 0.04191062;
	D_sp_hfl = 0.02470569;
	theta_k_ref = 0.11244578;
	// stance preparation SIMBICON-style feedback parameters
	theta_h_ref0 = -0.16986034; //TODO //0.60620732;
	c_d = 0.0; //0.5;
	c_v = 0.0; //0.2;
	// swing initiation and stance preparation offsets
	d_sp = -0.07115247;
	d_si = 0.37523836 ;
	//additional parameters
	k_THETA = 5.15958768; //1.15;
	k_theta = 6.68941640;
	phi_off_pk = 0.12978405;

}

/*! \brief sets optimization values for anytime after constructor 
 * (e.g. after x strikes)
 */
void StimWangCtrl::set_opti_delayed()
{
	// Use Matsuoka to optimize 6 parameters - opti_set() occured at constructor
	ghost_osc->delayed_opti_set();
	ghost_osc->update_speed_oscillos(); // update theta_*_ref's

	// Retrieve update values for trunk lean and hip angle, SOL and GAS gains
	// HIP CONTROL
	theta_ref = ghost_osc->get_theta_trunk_ref();
	theta_h_ref0 = ghost_osc->get_theta_hip_ref();
	// ANKLE CONTROL
	G_sol = ghost_osc->get_G_sol();
	G_sol_ta = ghost_osc->get_G_sol_ta();
	G_gas = ghost_osc->get_G_gas();
	// KNEE EXTENSION CONTROL
	G_vas = ghost_osc->get_G_vas();	// Shock absorbing on stance, generate thrust 
	k_theta = ghost_osc->get_k_theta(); // Prevents hyperextension

	// DEBUG
	// printf("Delayed opti set, check parameter values\n");
	// printf("theta_ref %f\n", theta_ref);
	// printf("theta_h_ref0 %f\n", theta_h_ref0);
	// printf("G_sol %f\n", G_sol);
	// printf("G_sol_ta %f\n", G_sol_ta);
	// printf("G_gas %f\n", G_gas);
	// printf("G_vas %f\n", G_vas);
	// printf("k_theta %f\n", k_theta);

}

/*! \brief destructor
 */
StimWangCtrl::~StimWangCtrl()
{
	delete delay_manager;
	delete opti_init;
}

/*! \brief main computation
 */
void StimWangCtrl::compute()
{
	compute_delay();

	update_oscillators();

	switch_results();

	compute_stimulation();
}

/*! \brief switch between know results and opti
 */
void StimWangCtrl::switch_results()
{
	if(ctrl_two_parts)
	{
		if((sw_st->get_nb_strikes() < nb_strikes_switch) && !flag_part1)
		{
			//opti_init->set_opti();
			set_opti_defaults();
			flag_part1 = 1;
		}
		else if((sw_st->get_nb_strikes() >= nb_strikes_switch) && !flag_part2)
		{
			set_opti_delayed();
			flag_part2 = 1;
		}
	}
}

/*! \brief compute delays in signals
 */
void StimWangCtrl::compute_delay()
{
	// update time
	delay_manager->set_t(inputs->get_t());

	// angles (delay)
	phi_k[R_ID]  = delay_manager->update_and_get(PHI_K_R,  pk[R_ID]->get_q());
	phi_k[L_ID]  = delay_manager->update_and_get(PHI_K_L,  pk[L_ID]->get_q());
	phip_k[R_ID] = delay_manager->update_and_get(PHIP_K_R, pk[R_ID]->get_qd());
	phip_k[L_ID] = delay_manager->update_and_get(PHIP_K_L, pk[L_ID]->get_qd());
	phi_h[R_ID]  = delay_manager->update_and_get(PHI_H_R,  ph[R_ID]->get_q());
	phi_h[L_ID]  = delay_manager->update_and_get(PHI_H_L,  ph[L_ID]->get_q());
	phip_h[R_ID] = delay_manager->update_and_get(PHIP_H_R, ph[R_ID]->get_qd());
	phip_h[L_ID] = delay_manager->update_and_get(PHIP_H_L, ph[L_ID]->get_qd());

	// l.ce (delay)
	lce_ta[R_ID]  = delay_manager->update_and_get(LCE_TA_R,  ta[R_ID]->get_lce());
	lce_ta[L_ID]  = delay_manager->update_and_get(LCE_TA_L,  ta[L_ID]->get_lce());
	lce_ham[R_ID] = delay_manager->update_and_get(LCE_HAM_R, ham[R_ID]->get_lce());
	lce_ham[L_ID] = delay_manager->update_and_get(LCE_HAM_L, ham[L_ID]->get_lce());
	lce_hfl[R_ID] = delay_manager->update_and_get(LCE_HFL_R, hfl[R_ID]->get_lce());
	lce_hfl[L_ID] = delay_manager->update_and_get(LCE_HFL_L, hfl[L_ID]->get_lce());

	// Fm (delay)
	F_sol[R_ID] = delay_manager->update_and_get(F_SOL_R, sol[R_ID]->get_Fm());
	F_sol[L_ID] = delay_manager->update_and_get(F_SOL_L, sol[L_ID]->get_Fm());
	F_vas[R_ID] = delay_manager->update_and_get(F_VAS_R, vas[R_ID]->get_Fm());
	F_vas[L_ID] = delay_manager->update_and_get(F_VAS_L, vas[L_ID]->get_Fm());
	F_gas[R_ID] = delay_manager->update_and_get(F_GAS_R, gas[R_ID]->get_Fm());
	F_gas[L_ID] = delay_manager->update_and_get(F_GAS_L, gas[L_ID]->get_Fm());
	F_ham[R_ID] = delay_manager->update_and_get(F_HAM_R, ham[R_ID]->get_Fm());
	F_ham[L_ID] = delay_manager->update_and_get(F_HAM_L, ham[L_ID]->get_Fm());
	F_glu[R_ID] = delay_manager->update_and_get(F_GLU_R, glu[R_ID]->get_Fm());
	F_glu[L_ID] = delay_manager->update_and_get(F_GLU_L, glu[L_ID]->get_Fm());

	// torso (delay)
	theta_torso = delay_manager->update_and_get(THETA_TORSO, inputs->get_theta_torso(1));
	omega_torso = delay_manager->update_and_get(OMEGA_TORSO, inputs->get_omega_torso(1));
}

/*! \brief compute the stimulations
 */
void StimWangCtrl::compute_stimulation()
{
	// set stimulations to minimal value
	for(int i=RIGHT_LEG_BODY; i<=LEFT_LEG_BODY; i++)
	{
		for(int j=0; j<parts[i]->get_nb_muscles(); j++)
		{
			Stim[i][j] = S_MIN;
		}
	}
	
	// Debugging: TODO
	//set_plot(phi_h[R_ID], "R hip phi");
	//set_plot(phip_h[R_ID], "hip angular speed");
	//set_plot(phi_k[R_ID], "knee angle");
	//set_plot(theta_k_ref, "knee ref angle");
	//set_plot(phip_k[R_ID], "knee angular speed");

	//set_plot(theta_torso, "torso angle");
	//set_plot(theta_ref, "theta ref");
	//set_plot(omega_torso, "torso omega");
	//set_plot(theta_toro_sw0, "torso sw 0");

	pitch_compute();

	roll_compute_min();

	yaw_compute_min();

	// limit stimulations
	for(int i=RIGHT_LEG_BODY; i<=LEFT_LEG_BODY; i++)
	{
		for(int j=0; j<parts[i]->get_nb_muscles(); j++)
		{
			Stim[i][j] = limit_range(Stim[i][j], S_MIN, S_MAX);
		}
	}
}

/*! \brief pitch muscles computation
 */
void StimWangCtrl::pitch_compute()
{
	double PD_torso_ham, PD_torso_glu, PD_torso_hfl;
	double p_vas;
	double theta_h_ref, d;
	int first_step;

	first_step = (sw_st->get_nb_strikes() < 1);
	if (Qq_match_wang || inital_pos)
	{
		first_step = 0; //no special ctrl for first step if no impedance ctrl
	}
 
	for (int i=0; i<NB_LEGS; i++)
	{
		// swing
		if (sw_st->is_swing_leg(i))
		{
			// SOL
			Stim[i][SOL_MUSCLE] = S0_sol_sw;

			// TA
			Stim[i][TA_MUSCLE] = S0_ta_sw + pos(G_ta_sw * ( (lce_ta[i] / l_opt_ta) - l_off_ta_sw));

			// GAS
			Stim[i][GAS_MUSCLE] = S0_gas_sw;

			// HAM
			//Stim[i][HAM_MUSCLE] = S0_ham_sw + G_ham * (F_ham[i] / F_max_ham); 

			// RF
			Stim[i][RF_MUSCLE] = S0_rf_sw;

			// TODO: Reset stance flag for this leg
			first_stance[i] = 1;

			if (stance_preparation(i) && ((sw_st->get_nb_strikes() != 0) || !first_step))
			{
				// compute hip target ankle
				if (i==R_ID)
				{
					//d = fwd_kin->get_r_COM_Lfoot(0);
				}
				else
				{
					//d = fwd_kin->get_r_COM_Rfoot(0);
				}
				//theta_h_ref = -(-theta_h_ref0 + c_d * d + c_v * speed_fwd_global);
				// VAS
				Stim[i][VAS_MUSCLE] = S0_vas_sw + pos(K_sp_vas * (phi_k[i] - theta_k_ref) + D_sp_vas * phip_k[i]);

				// GLU
				//Stim[i][GLU_MUSCLE] = S0_glu_sw + neg(K_sp_glu * (phi_h[i] - theta_h_ref) + D_sp_glu * phip_h[i]);

				// HFL
				//Stim[i][HFL_MUSCLE] = S0_hfl_sw + pos(K_sp_hfl * (phi_h[i] - theta_h_ref) + D_sp_hfl * phip_h[i]);
			}
			else
			{
				// VAS
				Stim[i][VAS_MUSCLE] = S0_vas_sw;

				// GLU
				//Stim[i][GLU_MUSCLE] = S0_glu_sw + G_glu * (F_glu[i] / F_max_glu);

				// HFL
				if (first_swing[i])
				{
					//theta_toro_sw0 = theta_torso;
					first_swing[i] = 0;
				}

				// HFL
				//Stim[i][HFL_MUSCLE] = S0_hfl_sw + k_THETA * (theta_toro_sw0 - theta_ref) + pos(G_hfl * ( (lce_hfl[i] / l_opt_hfl) - l_off_hfl)) - pos(G_ham_hfl * ( (lce_ham[i] / l_opt_ham) - l_off_ham_hfl));
			}

			// Plots
			if (i == R_ID) 
			{
				// plot right leg stims
				//set_plot(Stim[R_ID][GLU_MUSCLE], "R GLU sw");	// TODO
				//set_plot(Stim[R_ID][HAM_MUSCLE], "R HAM sw");    // TODO
				//set_plot(Stim[R_ID][HFL_MUSCLE], "R HFL sw");    // TODO
				//set_plot(Stim[R_ID][RF_MUSCLE], "R RF sw");    // TODO
				
				//set_plot(0.0, "R GLU st");	// TODO
				//set_plot(0.0, "R HAM st");    // TODO
				//set_plot(0.0, "R HFL st");    // TODO
				//set_plot(0.0, "R RF st");    // TODO
			}
			else
			{
				
				// plot left leg stims
				//set_plot(Stim[L_ID][GLU_MUSCLE], "L GLU sw");	// TODO
				//set_plot(Stim[L_ID][HAM_MUSCLE], "L HAM sw");    // TODO
				//set_plot(Stim[L_ID][HFL_MUSCLE], "L HFL sw");    // TODO
				//set_plot(Stim[L_ID][RF_MUSCLE], "L RF sw");    // TODO
				
				//set_plot(0.0, "L GLU st");	// TODO
				//set_plot(0.0, "L HAM st");    // TODO
				//set_plot(0.0, "L HFL st");    // TODO
				//set_plot(0.0, "L RF st");    // TODO
				
			}
			
		}
		// stance
		else
		{
			if (first_stance[i])
			{
				//set_plot(phi_k[i], "knee ang. st.");
				first_stance[i] = 0;
			}
			// SOL
			Stim[i][SOL_MUSCLE] = S0_sol_st + G_sol * (F_sol[i] / F_max_sol);

			if(first_step) // first step
			{
				//Stim[i][SOL_MUSCLE] = S_MIN;
			}

			// TA
			Stim[i][TA_MUSCLE] = S0_ta_st - G_sol_ta * (F_sol[i] / F_max_sol) + pos(G_ta_st * ( (lce_ta[i] / l_opt_ta) - l_off_ta_st));

			if(first_step) // first step
			{
				//Stim[i][TA_MUSCLE] = S_MAX;
			}

			// GAS
			Stim[i][GAS_MUSCLE] = S0_gas_st + G_gas * (F_gas[i] / F_max_gas);

			if(first_step) // first step
			{
				//Stim[i][GAS_MUSCLE] = S_MIN;
			}

			// VAS
			Stim[i][VAS_MUSCLE] = S0_vas_st + G_vas * (F_vas[i] / F_max_vas);

			p_vas = k_theta * (phi_k[i] - phi_off_pk);

			if ( (p_vas < 0.0) && (phip_k[i] < 0.0))
			{
				Stim[i][VAS_MUSCLE] += p_vas;
			}

			// HAM
			//Stim[i][HAM_MUSCLE] = S0_ham_st + pos(K_ham * (theta_torso - theta_ref) + D_ham * omega_torso);

			if (tr_st->get_trailing_leg(i)) // trailing leg in double support
			{
				//Stim[i][HAM_MUSCLE] = S0_ham_st;
			}

			//RF
			Stim[i][RF_MUSCLE] = S0_rf_st;

			// GLU
			//Stim[i][GLU_MUSCLE] = S0_glu_st + pos(K_glu * (theta_torso - theta_ref) + D_glu * omega_torso);
			
			if (tr_st->get_trailing_leg(i)) // trailing leg in double support
			{
				//Stim[i][GLU_MUSCLE] = S0_glu_st;
			}

			// HFL
			//Stim[i][HFL_MUSCLE] = S0_hfl_st + neg(K_hfl * (theta_torso - theta_ref) + D_hfl * omega_torso);
			
			if (tr_st->get_trailing_leg(i)) // trailing leg in double support
			{
				//Stim[i][HFL_MUSCLE] = S0_hfl_st;
			}

			first_swing[i] = 1; // re-initialize flag to enter once at the beginning on the swing phase
				
			if (swing_initiation(i) && ((sw_st->get_nb_strikes() != 0) || !first_step))
			{
				// VAS
				Stim[i][VAS_MUSCLE] =  limit_range(Stim[i][VAS_MUSCLE], S_MIN, S_MAX) - si_vas;

				// RF
				Stim[i][RF_MUSCLE] = limit_range(Stim[i][RF_MUSCLE], S_MIN, S_MAX) + si_rf;

				// GLU
				//Stim[i][GLU_MUSCLE] = limit_range(Stim[i][GLU_MUSCLE], S_MIN, S_MAX) - si_glu;
				
				// HFL
				//Stim[i][HFL_MUSCLE] = limit_range(Stim[i][HFL_MUSCLE], S_MIN, S_MAX) + si_hfl;
			}

			// Plots

			if (i == R_ID)
			{
				//set_plot(Stim[R_ID][GLU_MUSCLE], "R GLU st");	// TODO
				//set_plot(Stim[R_ID][HAM_MUSCLE], "R HAM st");    // TODO
				//set_plot(Stim[R_ID][HFL_MUSCLE], "R HFL st");    // TODO
				//set_plot(Stim[R_ID][RF_MUSCLE], "R RF st");    // TODO
				
				//set_plot(0.0, "R GLU sw");	// TODO
				//set_plot(0.0, "R HAM sw");    // TODO
				//set_plot(0.0, "R HFL sw");    // TODO
				//set_plot(0.0, "R RF sw");    // TODO
			}
			else
			{
				
				//set_plot(Stim[L_ID][GLU_MUSCLE], "L GLU st");	// TODO
				//set_plot(Stim[L_ID][HAM_MUSCLE], "L HAM st");    // TODO
				//set_plot(Stim[L_ID][HFL_MUSCLE], "L HFL st");    // TODO
				//set_plot(Stim[L_ID][RF_MUSCLE], "L RF st");    // TODO
				
				//set_plot(0.0, "L GLU sw");	// TODO
				//set_plot(0.0, "L HAM sw");    // TODO
				//set_plot(0.0, "L HFL sw");    // TODO
				//set_plot(0.0, "L RF sw");    // TODO
				
			} 
			
		}

		// Stance+swing plots
		
		//set_plot(Stim[R_ID][GLU_MUSCLE], "R GLU");	// TODO
		//set_plot(Stim[R_ID][HAM_MUSCLE], "R HAM");    // TODO
		//set_plot(Stim[R_ID][HFL_MUSCLE], "R HFL");    // TODO
		//set_plot(Stim[R_ID][RF_MUSCLE], "R RF");    // TODO
		

		//set_plot(Stim[L_ID][GLU_MUSCLE], "L GLU");	// TODO
		//set_plot(Stim[L_ID][HAM_MUSCLE], "L HAM");    // TODO
		//set_plot(Stim[L_ID][HFL_MUSCLE], "L HFL");    // TODO
		//set_plot(Stim[L_ID][RF_MUSCLE], "L RF");    // TODO
		

		// Overwrite stims
		// TODO: Plot ghost_osc y[4]
		double y1 = ghost_osc->get_y_pos(0);
		double y2 = ghost_osc->get_y_pos(1);
		double y3 = ghost_osc->get_y_pos(2);
		double y4 = ghost_osc->get_y_pos(3);
		double y5 = ghost_osc->get_y_pos(4);
		double y6 = ghost_osc->get_y_pos(5);
	
		// compute hip target ankle
		if (i==R_ID)
		{
			d = fwd_kin->get_r_COM_Lfoot(0);
		}
		else
		{
			d = fwd_kin->get_r_COM_Rfoot(0);
		}
		// SIMBICON law, converts hip reference angle	
		//theta_h_ref = -(-theta_h_ref0 + c_d * d + c_v * speed_fwd_global);
		theta_h_ref = theta_h_ref0;
		//set_plot(theta_h_ref, "theta_h_ref");
		if ( inputs->get_t() > cpg_ctrl_thresh_t ) /* Overwrite stims after thresh -> always verified */
		{
			// Set flag, indicating cpg control initiated
			cpg_ctrl_active = 1;

			/*
			set_plot(y1, "y1");
			set_plot(y2, "y2");
			set_plot(y3, "y3");
			set_plot(y4, "y4");
			set_plot(y5, "y5");
			set_plot(y6, "y6");//*/



			if (i==R_ID) 
			{
				if (y5)    /* N5 positive - PD torso active (stance)*/
				{
					Stim[i][HFL_MUSCLE] = 
						S0_hfl_st + neg(K_hfl * (theta_torso - theta_ref) + D_hfl * omega_torso);
					Stim[i][GLU_MUSCLE] = 
						S0_glu_st + pos(K_glu * (theta_torso - theta_ref) + D_glu * omega_torso);
					Stim[i][HAM_MUSCLE] = 
						S0_ham_st + pos(K_ham * (theta_torso - theta_ref) + D_ham * omega_torso);
				}
				else if ((y1 || y2)) /* N1 and N2 positive - cpg-controlled HFL*/
				{
					Stim[i][HFL_MUSCLE] = 
								k_HFLrun1 * y1 + k_HFLrun2 * y2;
					// Zero-out GLU when y1 is positive, FF during swing
					Stim[i][GLU_MUSCLE] = (y1) ? S_MIN : S0_glu_sw + G_glu * (F_glu[i] / F_max_glu); 
					Stim[i][HAM_MUSCLE] = (y1) ? S_MIN : S0_ham_sw + G_ham * (F_ham[i] / F_max_ham); 
				} 
				else if (y4)    /* N4 (N6, too) positive - PD hip active (late swing), cpg-controlled HAM*/
				{
					Stim[i][HFL_MUSCLE] = 
						S0_hfl_sw + pos(K_sp_hfl * (phi_h[i] - theta_h_ref) + D_sp_hfl * phip_h[i]);
					Stim[i][GLU_MUSCLE] = 
						S0_glu_sw + neg(K_sp_glu * (phi_h[i] - theta_h_ref) + D_sp_glu * phip_h[i]);
					Stim[i][HAM_MUSCLE] = 
						k_HAMrun * y6;
						/* Force-feedback control of HAM in late-swing
						S0_ham_sw + G_ham * (F_ham[i] / F_max_ham); 
						*/
				}
				else 
				{
					Stim[i][HFL_MUSCLE] = S_MIN; 
					Stim[i][GLU_MUSCLE] = S_MIN; 
					Stim[i][HAM_MUSCLE] = S_MIN; 
				}

				
				// plot right leg stims
				//set_plot(Stim[R_ID][GLU_MUSCLE], "R GLU cpg");    // TODO
				//set_plot(Stim[R_ID][HAM_MUSCLE], "R HAM cpg");    // TODO
				//set_plot(Stim[R_ID][HFL_MUSCLE], "R HFL cpg");    // TODO
			}
			else if (i==L_ID) 
			{
				if (y2)    /* N2 positive - PD control of torso (stance)*/
				{
					Stim[i][HFL_MUSCLE] = 
						S0_hfl_st + neg(K_hfl * (theta_torso - theta_ref) + D_hfl * omega_torso);
					Stim[i][GLU_MUSCLE] = 
						S0_glu_st + pos(K_glu * (theta_torso - theta_ref) + D_glu * omega_torso);
					Stim[i][HAM_MUSCLE] = 
						S0_ham_st + pos(K_ham * (theta_torso - theta_ref) + D_ham * omega_torso);
				}
				else if (y4 || y5) /* N4 and N5 - cpg-controlled HFL, 0'd GLU*/
				{
					Stim[i][HFL_MUSCLE] =
						k_HFLrun1 * y4 + k_HFLrun2 * y5;
					// Zero-out GLU when y4 is positive, FF during swing
					Stim[i][GLU_MUSCLE] = (y4) ? S_MIN : S0_glu_sw + G_glu * (F_glu[i] / F_max_glu); 
					Stim[i][HAM_MUSCLE] = (y4) ? S_MIN : S0_ham_sw + G_ham * (F_ham[i] / F_max_ham); 
				}
				else if (y1) /* N1 (N3, too) positive - PD control of hip, cpg-control of HAM (late swing)*/
				{
					Stim[i][HFL_MUSCLE] = 
						S0_hfl_sw + pos(K_sp_hfl * (phi_h[i] - theta_h_ref) + D_sp_hfl * phip_h[i]);
					Stim[i][GLU_MUSCLE] = 
						S0_glu_sw + neg(K_sp_glu * (phi_h[i] - theta_h_ref) + D_sp_glu * phip_h[i]);
					Stim[i][HAM_MUSCLE] = 
						k_HAMrun * y3;
						/* Force-feedback control of HAM in late-swing
						S0_ham_sw + G_ham * (F_ham[i] / F_max_ham); 
						*/
				}
				else 
				{
					Stim[i][HFL_MUSCLE] = S_MIN; 
					Stim[i][GLU_MUSCLE] = S_MIN; 
					Stim[i][HAM_MUSCLE] = S_MIN; 
				}
				//set_plot(Stim[L_ID][GLU_MUSCLE], "L GLU cpg");    // TODO
				//set_plot(Stim[L_ID][HAM_MUSCLE], "L HAM cpg");    // TODO
				//set_plot(Stim[L_ID][HFL_MUSCLE], "L HFL cpg");    // TODO
			}
		}
	}
}

/*! \brief pitch muscles computation: minimal stimulations
 */
void StimWangCtrl::pitch_compute_min()
{
	for (int i=0; i<NB_LEGS; i++)
	{
		Stim[i][SOL_MUSCLE] = S_MIN;
		Stim[i][TA_MUSCLE]  = S_MIN;
		Stim[i][GAS_MUSCLE] = S_MIN;
		Stim[i][VAS_MUSCLE] = S_MIN;
		Stim[i][HAM_MUSCLE] = S_MIN;
		Stim[i][GLU_MUSCLE] = S_MIN;
		Stim[i][HFL_MUSCLE] = S_MIN;
		Stim[i][RF_MUSCLE] = S_MIN;
	}
}

/*! \brief roll muscles computation: minimal stimulations
 */
void StimWangCtrl::roll_compute_min()
{
	for (int i=0; i<NB_LEGS; i++)
	{
		Stim[i][HAB_MUSCLE] = S_MIN;
		Stim[i][HAD_MUSCLE] = S_MIN;
		Stim[i][EVE_MUSCLE] = S_MIN;
		Stim[i][INV_MUSCLE] = S_MIN;
	}
}

/*! \brief yaw muscles computation: minimal stimulations
 */
void StimWangCtrl::yaw_compute_min()
{
	for (int i=0; i<NB_LEGS; i++)
	{
		Stim[i][HER_MUSCLE] = S_MIN;
		Stim[i][HIR_MUSCLE] = S_MIN;
	}
}

/*! \brief return 1 if stance preparation starts (end of swing phase), 0 otherwise
 */
int StimWangCtrl::stance_preparation(int leg_id)
{
	double d; // normalized horizontal distance between COM and foot

	switch(leg_id)
	{
		case R_ID : 
			d = fwd_kin->get_r_COM_Rfoot(0) / LEG_LENGTH;
			break;
		case L_ID : 
			d = fwd_kin->get_r_COM_Lfoot(0) / LEG_LENGTH;
			break;

		default:
			std::cout << "Error: unknown leg id : " << leg_id << " !" << std::endl;
			exit(EXIT_FAILURE);
	}

	/*
	switch(leg_id)
	{
		case R_ID : 
			return (ghost_osc->get_y_pos(3) || ghost_osc->get_y_pos(5));
			break;
		case L_ID : 
			return (ghost_osc->get_y_pos(0) || ghost_osc->get_y_pos(2));
			break;

		default:
			std::cout << "Error: unknown leg id : " << leg_id << " !" << std::endl;
			exit(EXIT_FAILURE);
	}//*/

	return (d < d_sp);
}

/*! \brief return 1 if swing initiation starts (end of stance phase), 0 otherwise
 */
int StimWangCtrl::swing_initiation(int leg_id)
{
	double d; // normalized horizontal distance between COM and foot

	switch(leg_id)
	{
		case R_ID : 
			d = fwd_kin->get_r_COM_Rfoot(0) / LEG_LENGTH;
			break;
		case L_ID : 
			d = fwd_kin->get_r_COM_Lfoot(0) / LEG_LENGTH;
			break;

		default:
			std::cout << "Error: unknown leg id : " << leg_id << " !" << std::endl;
			exit(EXIT_FAILURE);
	}

	/*
	switch(leg_id)
	{
		case R_ID : 
			return (ghost_osc->get_y_pos(0) || ghost_osc->get_y_pos(2));
			break;
		case L_ID : 
			return (ghost_osc->get_y_pos(3) || ghost_osc->get_y_pos(5));
			break;

		default:
			std::cout << "Error: unknown leg id : " << leg_id << " !" << std::endl;
			exit(EXIT_FAILURE);
	}//*/

	return ((d > d_si) || (sw_st->is_double_support() && d > 0.0));
}

/*! \brief update oscillator gains for velocity tracking */
void StimWangCtrl::update_oscillators()
{
	ghost_osc->update(inputs->get_t());

	// Retrieve updated values for cpg gains, trunk lean and hip ref angles, SOL and GAS gains
	// CPG CONTROL
	k_HFLrun1 = ghost_osc->get_k_HFLrun1();
	k_HFLrun2 = ghost_osc->get_k_HFLrun2();
	k_HAMrun = ghost_osc->get_k_HAMrun();

	// HIP CONTROL
	theta_ref = ghost_osc->get_theta_trunk_ref();
	theta_h_ref0 = ghost_osc->get_theta_hip_ref();
	// ANKLE CONTROL
	G_sol = ghost_osc->get_G_sol();
	G_sol_ta = ghost_osc->get_G_sol_ta();
	G_gas = ghost_osc->get_G_gas();
	// KNEE EXTENSION CONTROL
	G_vas = ghost_osc->get_G_vas();	// Shock absorbing on stance, generate thrust 
	k_theta = ghost_osc->get_k_theta(); // Prevents hyperextension

	/*
	set_plot(k_HFLrun1, "KHFL1");
	set_plot(k_HFLrun2, "KHFL2");
	set_plot(k_HAMrun, "KHAM");
	set_plot(theta_ref, "trunk_th");
	set_plot(theta_h_ref0, "hip_th");
	set_plot(G_sol, "G_sol");
	set_plot(G_sol_ta, "G_sol_ta");
	set_plot(G_gas, "G_gas");
	set_plot(G_vas, "G_vas");
	set_plot(k_theta, "Ktheta");//*/
}


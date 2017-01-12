#include "OptiResults.hh"
#include "StimWangCtrl.hh"
#include "JointsInit.hh"
#include "MatsuokaSixN.hh"

/*! \brief constructor
 */
OptiResults::OptiResults()
{

}

/*! \brief destructor
 */
OptiResults::~OptiResults()
{

}

/*! \brief set optimization parameters
 */
void OptiResults::set_opti()
{
    StimWangCtrl *stim_wang = static_cast<StimWangCtrl*>(stim_ctrl);
    MatsuokaSixN *g_osc = static_cast<MatsuokaSixN*>(stim_wang->get_ghost_osc());

    // Uses "opti_set" function family in order to delay setting these params
    // until after Ctrl construction
    g_osc->opti_set_P_tau(0.04550037 ); // Taken from master commit: "All_1 results, 1459 score"
	// HIP CONTROL
	g_osc->opti_set_P_theta_trunk( 0.06053428 ); // TODO: trunk lean 
	g_osc->opti_set_P_theta_hip( 0.37451764 ); // TODO: made its lower bound smaller, come back to this later
    
	// CPG CONTROL
    g_osc->opti_set_k_HFLrun1( 1.48182842);	// Higher bound
    g_osc->opti_set_k_HFLrun2( 9.02060054);	// Higher bound
    g_osc->opti_set_k_HAMrun( 0.50538843);

	// ANKLE CONTROL
	g_osc->opti_set_P_G_SOL( 1.97774871 );	// Lower bounds
	g_osc->opti_set_P_G_SOL_TA( 4.79443890 );// Prevent tripping
	g_osc->opti_set_P_G_GAS( 15.70603234 );
	
	// KNEE CONTROL
	g_osc->opti_set_P_G_VAS( 2.29627266 );	// Shock absorption, v. thrust
	g_osc->opti_set_P_k_theta( 9.59053435);	// Prevent hyperextension
}
//energy: 8.97732 [J/(m*kg)]
//speed : 1.29445 [m/s]
//period: 0.47833 [s]
//length: 0.606225 [m]
//one leg stance phase: 31.4008 [%]
//double support: 0 [%]
//tot flight: 37.205 [%]


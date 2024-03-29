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
    g_osc->opti_set_P_tau(0.04408793 ); // Taken from master commit: "All_1 results, 1459 score"
	// HIP CONTROL
	g_osc->opti_set_P_theta_trunk( 0.03667824 ); // TODO: trunk lean 
	g_osc->opti_set_P_theta_hip( 0.16571730 ); // TODO: made its lower bound smaller, come back to this later
    
	// CPG CONTROL
    g_osc->opti_set_k_HFLrun1( 7.46480136);	// Higher bound
    g_osc->opti_set_k_HFLrun2( 11.62397394);	// Higher bound
    g_osc->opti_set_k_HAMrun( 5.82683115);

	// ANKLE CONTROL
	g_osc->opti_set_P_G_SOL( 2.73405709 );	// Lower bounds
	g_osc->opti_set_P_G_SOL_TA( 6.13702195 );// Prevent tripping
	g_osc->opti_set_P_G_GAS( 17.81075627 );
	
	// KNEE CONTROL
	g_osc->opti_set_P_G_VAS( 3.73639101 );	// Shock absorption, v. thrust
	g_osc->opti_set_P_k_theta( 14.02330322);	// Prevent hyperextension
}

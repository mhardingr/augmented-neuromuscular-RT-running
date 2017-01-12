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
    g_osc->opti_set_P_tau(0.04643019 ); // Taken from master commit: "All_1 results, 1459 score"
	// HIP CONTROL
	g_osc->opti_set_P_theta_trunk( 0.07579872 ); // TODO: trunk lean 
	g_osc->opti_set_P_theta_hip( 0.08166239 ); // TODO: made its lower bound smaller, come back to this later
    
	// CPG CONTROL
    g_osc->opti_set_k_HFLrun1( 5.45502321);
    g_osc->opti_set_k_HFLrun2( 6.76519931);
    g_osc->opti_set_k_HAMrun( 3.16770928);

	// ANKLE CONTROL
	g_osc->opti_set_P_G_SOL( 3.99111179 );	// TODO: Original bounds
	g_osc->opti_set_P_G_SOL_TA( 5.80995683 );// Prevent tripping
	g_osc->opti_set_P_G_GAS( 4.97505824 );

	// KNEE CONTROL
	g_osc->opti_set_P_G_VAS( 1.56252814 );	// Shock absorption, v. thrust
	g_osc->opti_set_P_k_theta( 14.66721583);	// Prevent hyperextension
}

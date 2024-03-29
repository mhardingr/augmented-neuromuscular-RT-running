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
    g_osc->opti_set_P_tau(0.04458599 ); // Taken from master commit: "All_1 results, 1459 score"
	// HIP CONTROL
	g_osc->opti_set_P_theta_trunk( 0.04610294 ); // TODO: trunk lean 
	g_osc->opti_set_P_theta_hip( 0.12348294 ); // TODO: made its lower bound smaller, come back to this later
    
	// CPG CONTROL
    g_osc->opti_set_k_HFLrun1( 6.62281606);	// Higher bound
    g_osc->opti_set_k_HFLrun2( 7.98866747);	// Higher bound
    g_osc->opti_set_k_HAMrun3( 6.48621756);

	// ANKLE CONTROL
	g_osc->opti_set_P_G_SOL( 2.48552686 );	// Lower bounds
	g_osc->opti_set_P_G_SOL_TA( 6.86052139 );// Prevent tripping
	g_osc->opti_set_P_G_GAS( 2.14716014 );
	
	// KNEE CONTROL
	g_osc->opti_set_P_G_VAS( 2.06983791 );	// Shock absorption, v. thrust
	g_osc->opti_set_P_k_theta( 9.44158925);	// Prevent hyperextension
}

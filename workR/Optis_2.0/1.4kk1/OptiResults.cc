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
    g_osc->opti_set_P_tau(0.04890590 ); // Taken from master commit: "All_1 results, 1459 score"
	// HIP CONTROL
	g_osc->opti_set_P_theta_trunk( 0.07277579 ); // TODO: trunk lean 
	g_osc->opti_set_P_theta_hip( 0.21326678 ); // TODO: made its lower bound smaller, come back to this later
    
	// CPG CONTROL
    g_osc->opti_set_k_HFLrun1( 6.04707720);	// Higher bound
    g_osc->opti_set_k_HFLrun2( 2.25449978);	// Higher bound
    g_osc->opti_set_k_HAMrun( 3.22527963);

	// ANKLE CONTROL
	g_osc->opti_set_P_G_SOL( 2.68250864 );	// Lower bounds
	g_osc->opti_set_P_G_SOL_TA( 1.35304783 );// Prevent tripping
	g_osc->opti_set_P_G_GAS( 9.25481373 );
	
	// KNEE CONTROL
	g_osc->opti_set_P_G_VAS( 1.06806469 );	// Shock absorption, v. thrust
	g_osc->opti_set_P_k_theta( 5.71777443);	// Prevent hyperextension
}
//energy: 8.8911 [J/(m*kg)]
//speed : 1.41982 [m/s]
//period: 0.463503 [s]
//length: 0.659737 [m]
//one leg stance phase: 32.3633 [%]
//double support: 0 [%]
//tot flight: 35.2522 [%]

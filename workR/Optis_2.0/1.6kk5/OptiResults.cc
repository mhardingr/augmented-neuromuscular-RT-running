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
    g_osc->opti_set_P_tau(0.04551283 ); // Taken from master commit: "All_1 results, 1459 score"
	// HIP CONTROL
	g_osc->opti_set_P_theta_trunk( 0.05171115 ); // TODO: trunk lean 
	g_osc->opti_set_P_theta_hip( 0.12245877 ); // TODO: made its lower bound smaller, come back to this later
    
	// CPG CONTROL
    g_osc->opti_set_k_HFLrun1( 6.04478571);	// Higher bound
    g_osc->opti_set_k_HFLrun2( 3.24343141);	// Higher bound
    g_osc->opti_set_k_HAMrun3( 3.79482167);

	// ANKLE CONTROL
	g_osc->opti_set_P_G_SOL( 2.17216206 );	// Lower bounds
	g_osc->opti_set_P_G_SOL_TA( 5.09447877 );// Prevent tripping
	g_osc->opti_set_P_G_GAS( 1.89712311 );
	
	// KNEE CONTROL
	g_osc->opti_set_P_G_VAS( 0.89797189 );	// Shock absorption, v. thrust
	g_osc->opti_set_P_k_theta( 12.96538083);	// Prevent hyperextension
}
//energy: 8.18611 [J/(m*kg)]
//speed : 1.58006 [m/s]
//period: 0.43584 [s]
//length: 0.691184 [m]
//one leg stance phase: 34.4167 [%]
//double support: 0 [%]
//tot flight: 31.1689 [%]


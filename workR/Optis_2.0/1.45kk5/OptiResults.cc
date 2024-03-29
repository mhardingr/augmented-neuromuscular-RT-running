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
    g_osc->opti_set_P_tau(0.04654490 ); // Taken from master commit: "All_1 results, 1459 score"
	// HIP CONTROL
	g_osc->opti_set_P_theta_trunk( 0.02206084 ); // TODO: trunk lean 
	g_osc->opti_set_P_theta_hip( 0.08497627 ); // TODO: made its lower bound smaller, come back to this later
    
	// CPG CONTROL
    g_osc->opti_set_k_HFLrun1( 3.50028006);	// Higher bound
    g_osc->opti_set_k_HFLrun2( 1.82767676);	// Higher bound
    g_osc->opti_set_k_HAMrun3( 3.94388207);

	// ANKLE CONTROL
	g_osc->opti_set_P_G_SOL( 2.89917615 );	// Lower bounds
	g_osc->opti_set_P_G_SOL_TA( 5.19909587 );// Prevent tripping
	g_osc->opti_set_P_G_GAS( 19.84552877 );
	
	// KNEE CONTROL
	g_osc->opti_set_P_G_VAS( 0.87570574 );	// Shock absorption, v. thrust
	g_osc->opti_set_P_k_theta( 9.00628295);	// Prevent hyperextension
}
//energy: 8.32206 [J/(m*kg)]
//speed : 1.43228 [m/s]
//period: 0.447898 [s]
//length: 0.63738 [m]
//one leg stance phase: 33.4903 [%]
//double support: 0 [%]
//tot flight: 33.0252 [%]


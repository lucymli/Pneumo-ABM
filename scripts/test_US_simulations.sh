rm -r test_US_simulations
python test_US_simulations.py
python generate_prevalence_trajectories.py -l test_US_simulations -d 1 -f 3650

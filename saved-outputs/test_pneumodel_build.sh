cd ../source
g++-6 -L/usr/local/lib -lboost_filesystem -lboost_program_options -lboost_system -fopenmp -I/usr/local/include *.cpp -o ../scripts/test_model_US_simulations

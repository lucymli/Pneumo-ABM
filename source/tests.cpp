// #define CATCH_CONFIG_MAIN 
// #include "catch.hpp"

// #include <cmath>
// #include <iostream>
// #include <numeric>
// #include <memory>
// #include <vector>

// #include "random_number_generator.h"
// #include "colonization.h"
// #include "host.h"
// #include "host_container.h"
// #include "event.h"
// #include "event_queue.h"
// #include "population.h"

// TEST_CASE( "RandomNumberGenerator", "" ) {
//   RandomNumberGenerator rng (1);
//   float N = 10000.0;

//   SECTION( "Bernoulli" ) {
//     int num_successes = 0;
//     float p = 0.7;
//     for (int i = 0 ; i < N; i++)
//       if (rng.draw_bernoulli(p)) num_successes++;
    
//     // examine observed probability of success
//     float p_hat = num_successes / N;
//     CHECK( p_hat < p + 0.05 );
//     CHECK( p_hat > p - 0.05 );

//     CHECK_THROWS( rng.draw_bernoulli(7) );
//   }

//   SECTION( "Uniform Integer" ) {
//     int a = 10;
//     int b = 15;
//     std::vector<int> counts (b + 1, 0);
//     for (int i = 0; i < N; i++) 
//       counts[rng.draw_uniform_int(a, b)]++;
    
//     // examine proportion of trials where we observe each integer
//     for (int i = 0; i < a; i++)
//       CHECK( counts[i] == 0);
//     for (int i = a; i < b + 1; i++) {
//       CHECK( counts[i] / N < (1.0 / (b - a)) + 0.05 ); 
//       CHECK( counts[i] / N > (1.0 / (b - a)) - 0.05 );
//     }
//   }

//   std::vector<double> weights = { 20, 50, 50 };
//   float sum_of_weights = std::accumulate(weights.begin(), weights.end(), 0);

//   SECTION( "Categorical" ) {  
//     std::vector<int> categorical_counts (weights.size(), 0);
//     for (int i = 0; i < N; i ++) 
//       categorical_counts[rng.draw_categorical(weights)]++;

//     // examine proportion of trials where we observe each integer
//     for (int i = 0; i < weights.size(); i++) {
//       CHECK( categorical_counts[i] / N < (weights[i] / sum_of_weights) + 0.05 );
//       CHECK( categorical_counts[i] / N > (weights[i] / sum_of_weights) - 0.05 );
//     }
//   }

//   SECTION( "Multinomial" ) {
//     std::vector<int> multinomial_counts (weights.size(), 0);
//     multinomial_counts = rng.draw_multinomial(N, weights);
    
//     // examine observed multinomial vector
//     for (int i = 0; i < weights.size(); i++) {
//       CHECK( multinomial_counts[i] / N < (weights[i] / sum_of_weights) + 0.05 );
//       CHECK( multinomial_counts[i] / N > (weights[i] / sum_of_weights) - 0.05 );
//     }
//   }

// add tests for POISSON

//   SECTION( "Exponential" ) {
//     float mean = 25;
//     std::vector<double> X (N, 0);
//     for (int i = 0; i < N; i++) 
//       X[i] = rng.draw_exponential(mean);
    
//     // test mean
//     float observed_mean = std::accumulate(X.begin(), X.end(), 0) / N;
//     CHECK( observed_mean < mean + 2 ); 
//     CHECK( observed_mean > mean - 2 );

//     // test variance is as we expect
//     std::vector<double> deltas_squared (N, 0);
//     for (int i = 0; i < N; i++) 
//       deltas_squared[i] = pow(X[i] - observed_mean, 2);
//     float observed_variance = std::accumulate(deltas_squared.begin(), deltas_squared.end(), 0) / N;
//     CHECK( observed_variance < pow(mean, 2) + 25 );
//     CHECK( observed_variance > pow(mean, 2) - 25 );
//   }
// }

// TEST_CASE( "Colonization", "" ) {
//   auto rng_ptr = std::make_shared<RandomNumberGenerator>(1);
//   Colonization::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");

//   // check that configuration parameters were read in correctly 
//   CHECK( Colonization::get_num_serotypes() == 3 );
//   CHECK( Colonization::get_kappa() == 1 );
//   CHECK( Colonization::get_epsilon() == 0.5 );
//   CHECK( Colonization::get_min_duration() == 20.0 );
//   CHECK( Colonization::get_max_duration() == 60.0 );
//   CHECK( Colonization::get_fitness_rank(0) == 1 );

//   // check that gammas were computed correctly
//   CHECK( Colonization::get_gamma(0) == 20.0 );
//   CHECK( Colonization::get_gamma(1) == 40.0 );
//   CHECK( Colonization::get_gamma(2) == 60.0 );

//   // test getters
//   Colonization c0 = Colonization(10, 0, 0, rng_ptr);
//   CHECK( c0.get_day_of_colonization() == 10);
//   CHECK( c0.get_serotype() == 0);

//   // day of recovery should always come after day of colonization
//   CHECK( c0.get_day_of_recovery() >= c0.get_day_of_colonization() );

//   // and duration should be at least the minimum
//   int duration = c0.get_day_of_recovery() - c0.get_day_of_colonization();
//   CHECK ( duration > Colonization::get_kappa() );

//   // more previous colonizations should lead to faster recovery, all else equal
//   Colonization c1 = Colonization(10, 0, 25, rng_ptr); 
//   CHECK( c1.get_day_of_recovery() <= c0.get_day_of_recovery() );

//   // even more previous colonizations should lead to even faster recovery, all else equal
//   Colonization c2 = Colonization(10, 0, 60, rng_ptr); 
//   CHECK( c2.get_day_of_recovery() <= c1.get_day_of_recovery() );

//   // a large serotype index that is beyond the number of serotypes should throw an exception
//   CHECK_THROWS( Colonization(10, 99999, 5, rng_ptr) );
// }

// TEST_CASE( "Host" ) {
//   auto rng_ptr = std::make_shared<RandomNumberGenerator>(1);
//   Colonization::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");
//   Host::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");

//   // configured properly?
//   CHECK( Host::get_maturity_age() == 15 );
//   CHECK( Host::get_mu() == 0.25 );
//   CHECK( Host::get_sigma() == 0.5);

//   Host h {0, 75 * 365, 16 * 365, 18 * 365, {25 * 365, 27 * 365}, rng_ptr};
  
//   SECTION( "Demographics" ) {
//     // check getters
//     CHECK( h.get_id() != Host::null_id );
//     CHECK( h.get_partner_id() == Host::null_id );
//     CHECK( h.get_household_id() == Host::null_household_id );
//     CHECK( h.get_day_of_birth() == 0 );
//     CHECK( h.get_day_of_death() == 75 * 365 );
//     CHECK( h.get_day_of_fledge() == 16 * 365 );
//     CHECK( h.get_day_of_partnering() == 18 * 365 );
//     CHECK( h.get_days_of_childbirth()[0] == 25 * 365 );
//     CHECK( h.get_days_of_childbirth()[1] == 27 * 365 );
//     CHECK_FALSE( h.is_fledged() );

//     // setters
//     CHECK( h.is_single() );
//     h.set_partner_id(3);
//     h.set_household_id(1);
//     h.set_fledged();
//     CHECK( h.get_partner_id() == 3 );
//     CHECK( h.get_household_id() == 1 );
//     CHECK( h.is_fledged() );

//     // demographic functions
//     CHECK_FALSE( h.is_single() );
//     CHECK( h.calculate_age(35 * 365 - 2) == 34 );
//     CHECK( h.calculate_age(35 * 365 + 1) == 35 );
//     CHECK( h.calculate_lifespan() == 75 );
//     CHECK_FALSE( h.is_adult(Host::get_maturity_age() * 365 - 10) );
//     CHECK( h.is_adult(Host::get_maturity_age() * 365) );
//   }

//   SECTION( "Colonization" ) {
//     // colonization by pneumo
//     for (int s = 0; s < Colonization::get_num_serotypes(); s++)
//       CHECK( h.num_colonizations(s) == 0 );
    
//     Colonization c1 = h.acquire_colonization(10 * 365, 0);
//     Colonization c2 = h.acquire_colonization(10 * 365 + 1, 2); 
//     CHECK( h.num_colonizations(0) == 1 );
//     CHECK( h.num_colonizations(2) == 1 );
//     CHECK( h.num_serotypes() == 2 );

//     // removing expired colonization
//     auto serotypes_removed = h.remove_expired_colonizations(c1.get_day_of_recovery()); // c1 is removed, but c2 should not be removed 
//     CHECK( serotypes_removed.size() == 1 );
//     serotypes_removed = h.remove_expired_colonizations(c2.get_day_of_recovery()); // c2 should now be removed 
//     CHECK( serotypes_removed.size() == 1 );
//     CHECK( serotypes_removed[0] == c2.get_serotype() );

//     // removing multiple expired colonizations
//     std::vector<int> recovery_dates;
//     for (int i = 0; i < 23; i++) {
//       Colonization c = h.acquire_colonization(10 * 365 + i, i % 3);
//       recovery_dates.push_back(c.get_day_of_recovery());
//     }
//     std::sort(recovery_dates.begin(), recovery_dates.end());
//     int k = 12;
//     serotypes_removed = h.remove_expired_colonizations(recovery_dates[k]);
//     CHECK( serotypes_removed.size() == k + 1 );

//     CHECK_NOTHROW( h.verify_num_colonizations() );
//   }
// }

// TEST_CASE( "Host Susceptibility" ) {
//   auto rng_ptr = std::make_shared<RandomNumberGenerator>(1);
//   Colonization::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");
//   Host::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");

//   Host h {0, 75 * 365, 16 * 365, 18 * 365, {25 * 365, 27 * 365}, rng_ptr};
//   SECTION( "No previous colonizations" ) {
//     for (int s = 0; s < Colonization::get_num_serotypes(); s++) 
//       CHECK( h.get_susceptibility(s) == 1.0 );
//   }
  
//   SECTION( "Effect of previous colonizations" ) {
//     double s0 = h.get_susceptibility(0); 
//     Colonization c = h.acquire_colonization(0, 0);
//     h.remove_expired_colonizations(c.get_day_of_recovery());
//     double s1 = h.get_susceptibility(0);
//     CHECK( s0 > s1 ); // protection for previously-seen serotype
//     CHECK( h.get_susceptibility(2) == 1); // no protection for other serotype
//   }

//   SECTION( "Effect of having a fitter incumbent serotype" ) {
//     Host a = Host {h};
//     Host b = Host {h};
//     Host c = Host {h};
//     a.acquire_colonization(0, 0); // fitness rank 1
//     b.acquire_colonization(0, 1); // fitness rank 2
//     c.acquire_colonization(0, 2); // fitness rank 3
//     CHECK( a.get_susceptibility(1) < b.get_susceptibility(1) );
//     CHECK( b.get_susceptibility(1) < c.get_susceptibility(1) );
//   }
// }

// TEST_CASE( "Event" ) {
//   Event e {0, 1, Event::Type::Childbirth};
//   CHECK( e.get_host_id() == 0 );
//   CHECK( e.get_day() == 1 );
//   CHECK( e.get_type() == Event::Type::Childbirth);
// }

// TEST_CASE( "EventQueue" ) {
//   auto rng_ptr = std::make_shared<RandomNumberGenerator>(1);
//   Host::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");
//   Colonization::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");

//   // adding events
//   EventQueue eq;
//   auto h = std::make_shared<Host>(0, 10000, 20, 30, std::vector<int> {40, 50}, rng_ptr);
//   int num_added = eq.schedule_life_events(h); // should add 5 events
//   CHECK( num_added == 5 );
//   Colonization c = h->acquire_colonization(21, 0);
//   num_added = eq.schedule_recovery_event(h, c.get_day_of_recovery()); // add one more
//   REQUIRE( c.get_day_of_recovery() < h->get_day_of_death());
//   CHECK( num_added == 1 ); 
//   CHECK( eq.size() == 6 );

//   // should not add events that occur after death
//   auto g  = std::make_shared<Host>(0, 50, 51, 52, std::vector<int> {55}, rng_ptr);
//   num_added = eq.schedule_life_events(g);
//   CHECK( num_added == 1 ); // just add the death event
//   Colonization d = g->acquire_colonization(53, 0);
//   num_added = eq.schedule_recovery_event(g, d.get_day_of_recovery()); // no recovery event added
//   CHECK( num_added == 0 );
//   CHECK( eq.size() == 7 );

//   // pop events
//   CHECK( eq.day_of_next_event() == 20 );
//   Event next = eq.pop(); // remove an event from the queue
//   CHECK( eq.size() == 6 );
//   CHECK( next.get_host_id() == h->get_id() );
//   CHECK( next.get_day() == 20 );
//   CHECK( next.get_type() == Event::Type::Fledge);

//   std::shared_ptr<Host> h_ptr {new Host{0, 10000, 20, 30, {40, 50}, rng_ptr}}; // add 5 more
//   eq.schedule_life_events(h_ptr);
//   CHECK( eq.size() == 11);
// }

// TEST_CASE( "Population" ) {
//   auto rng_ptr = std::make_shared<RandomNumberGenerator>(1);
//   Population::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration", "test_config.json");
//   Host::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");
//   Colonization::configure("/Users/ocsicnarf/Github/Pneumo-ABM/test-configuration/test_config.json");

//   Population pop {rng_ptr};
//   // initialize and size
//   int N = 5000;
//   std::vector<int> host_ids = pop.initialize(N);
//   CHECK( pop.size() == N );
  
//   // find
//   auto host = pop.find_host(host_ids[5]);
//   int host_id = host->get_id();
//   CHECK( host_id == host_ids[5] );
  
//   SECTION( "Demographics" ) {
//     std::vector<int> num_hosts_by_age = pop.count_hosts_by_age(0);
//     // spot checks
//     for (int a : {1, 25, 48, 56, 57}) {
//       double p = Population::get_initial_age_pmf()[a];
//       double se = sqrt(N * p * (1 - p));
//       CHECK( num_hosts_by_age[a] < N * p + 2 * se );
//       CHECK( num_hosts_by_age[a] > N * p - 2 * se );
//     }
//     std::vector<int> num_hh = pop.count_households_by_size();
//     int total = 0;
//     for(int size = 0; size < num_hh.size(); size++) {
//       total += num_hh[size] * size;
//       if (size != 1 && size != 2) {
//         CHECK( num_hh[size] == 0 );
//       }
//     }
//     CHECK( total == N );
//   }

//   SECTION( "Event Handlers" ) { 
//     // event handlers
//     // birth
//     int new_id = pop.birth_host(host_ids[5], 0);
//     auto new_host = pop.find_host(new_id);
//     CHECK( new_host->get_household_id() == host->get_household_id()  );
//     CHECK( new_host->get_day_of_birth() == 0 );
//     // fledge
//     CHECK_FALSE( new_host->is_fledged() );
//     int old_hhid = new_host->get_household_id();
//     pop.fledge_host(new_host->get_id());
//     CHECK( pop.find_host(new_id)->is_fledged() );
//     CHECK( pop.find_host(new_id)->get_household_id() != old_hhid );
//     // partner
//     int i = 0;
//     auto initiator = pop.find_host(host_ids[i]);
//     while( !initiator->is_adult(0) ) {
//       initiator = pop.find_host(host_ids[i++]);
//     }
//     int initiator_id = initiator->get_id();
//     REQUIRE ( initiator->is_adult(0) );
//     int partner_id = pop.partner_host(initiator_id, 0);
//     auto partner = pop.find_host(partner_id);
//     REQUIRE (partner_id != Host::null_id ); // everyone is single. so partner should be available
//     CHECK_FALSE( pop.find_host(initiator_id)->is_single() );
//     CHECK_FALSE( pop.find_host(partner_id)->is_single() );
//     CHECK( pop.find_host(initiator_id)->get_household_id() == 
//            pop.find_host(partner_id)->get_household_id() );
//     // remove
//     CHECK ( pop.size() == N + 1 );
//     pop.remove_host(new_id);
//     CHECK_FALSE( pop.has_host(new_id) );
//     CHECK( pop.size() == N );
//   }

//   SECTION( "Colonization/Statistics" ) { 
//     // seed colonization
//     auto id_times = pop.seed_colonizations(1, 0);
//     auto num_colonizations = pop.count_colonizations();
//     int total = std::accumulate(num_colonizations.begin(), num_colonizations.end(), 0);
//     CHECK( id_times.size() ==  total);
//     CHECK( num_colonizations.size() == Colonization::get_num_serotypes() );
//     double p_col = Population::get_initial_colonization_prob();
//     double se_col = sqrt(N * p_col * 1 - p_col);
//     for (int s = 0; s < Colonization::get_num_serotypes(); s++) {
//       if (s == 1) {
//         CHECK( num_colonizations[s] < N * p_col + 2 * se_col );
//         CHECK( num_colonizations[s] > N * p_col - 2 * se_col );
//       } else {
//         CHECK( num_colonizations[s] == 0 );
//       }
//     }

//     auto num_colonizations_by_age = pop.count_colonizations_by_age(0);
//     auto num_hosts_by_age = pop.count_hosts_by_age(0);
//     int grand_sum = 0;
//     for(int a = 0; a < num_colonizations_by_age.size(); a++) {
//       auto nc = num_colonizations_by_age[a];
//       int sum = std::accumulate(nc.begin(), nc.end(), 0);
//       grand_sum += sum;
//       int n_age = num_hosts_by_age[a];
//       double se_age = sqrt(n_age * p_col * (1 - p_col));
//       CHECK( sum <= p_col * n_age + 5 * se_age);
//       CHECK( sum >= p_col * n_age - 5 * se_age);
//     }
//     CHECK( grand_sum == total );
    
//     // recover all hosts
//     int max_rec_day = 0;
//     for(auto pair : id_times) {
//       max_rec_day = std::max(pair.second, max_rec_day);
//     }
//     for(auto pair : id_times) {
//       int id = pair.first;
//       pop.recover_host(id, max_rec_day);
//     }
//     num_colonizations = pop.count_colonizations();
//     total = std::accumulate(num_colonizations.begin(), num_colonizations.end(), 0);
//     CHECK( total == 0 );


//     CHECK_NOTHROW( pop.verify_num_colonizations() );
//     // for (int day = 0; day < 365; day++) {
//     //   for (int s = 0; s < Colonization::get_num_serotypes(); s++) {
//     //     auto result = pop.colonize_hosts(s, day);
//     //     std::cout << result.size() << ',';
//     //   }
//     //   std::cout << std::endl;
//     // }
//   }
// }
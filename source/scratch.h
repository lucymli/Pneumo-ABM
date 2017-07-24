/* configuration.h
 * Helper function to load parameters from files
 */
/* in .cpp */
//#include <boost/property_tree/ptree.hpp>
// #include <boost/property_tree/json_parser.hpp>
#include <string>
namespace config {
  load_colonization_config(std::string filepath);
  load_host_config(std::string filepath);
  load_population_config(std::string filepath);
}
/* random_draws.h
 * Random draws from different distribution
 */
#include <random>

class RandomDraws {
public:
  void seed(int seed);
  int bernoulli_draw(double p);
  int uniform_int_draw(int a, int b);
  int categorical_draw(std::vector<double> weights);
  std::vector<int> multinomial_draw(int num_trials, std::vector<double> weights);
  double exponential_draw (double mean);
private:
  std::mt19937 engine_; // A Mersenne twister PRNG
};

/* colonization.h
 * Bare minimum needed to describe a pneumococcus/h. influenzae colonization. Does not need to know about any other class.
 */
#include <vector>
#include "random_number_generator.h"
// in .cpp #include "configuration.h"

class Colonization {
public:
  Colonization(int day_of_colonization, int serotype, int strain, int num_previous_colonizations) : // number of previous colonizations by this serotype
    day_of_colonization_(day_of_colonization),
    serotype_(serotype),
    strain_(strain);            

  int get_day_of_colonization() const;
  int get_day_of_recovery() const;
  int get_serotype() const;
  int get_strain() const;

  int set_day_of_recovery(int day_of_recovery); // e.g. if hflu clears an colonization, set day_of_recovery = current_day
private:
  int calculate_colonization_length(int num_previous_colonizations); // depends on previous colonizations, a reflection
                                                               // of acquired immunity  
  int serotype_;         // a special "serotype" for hflu? maybe not yet.
  int strain_;           // used to distinguish between multiple colonizations of the same serotype
  int day_of_colonization_; // cannot be changed
  int day_of_recovery_;  // this may need to be updated
  
//  static const int HFLU_INDEX = -1; // sentinel value

  // to calculate duration of colonization
  static const int KAPPA; // minimum duration of colonization
  static const int EPSILON; // controls strength of acquired immunity
  static const std::vector<double> MEAN_DURATIONS; // indexed by serotype
  static const int NUM_SEROTYPES; // initialize from parameters in a file
  // something to convert from serotype to fitness rank ???

  static RandomNumberGenerator& random_number_generator_; // initialize in main?
};
/*
in .cpp
const int num_serotypes = read_from_file(my_params_files.txt);
*/

/* host.h
 * Describes a host, including timing of life events and current/past colonizations. 
 */
#include <vector>
#include "colonization.h"
#include "random_number_generator.h"

class Host {
public:
  Host(int day_of_birth, int day_of_death, int day_of_fledge, 
       int day_of_partnering, std::vector<int> days_of_childbirth) :
    day_of_birth_(day_of_birth),
    day_of_death_(day_of_death),
    day_of_fledge_(day_of_fledge),
    day_of_partnering_(day_of_partnering),
    days_of_childbirth_(days_of_childbirth);

  // life events
  int get_day_of_birth() const;
  int get_day_of_death() const;
  int get_day_of_fledge() const;
  int get_day_of_partnering() const;
  std::vector<int> get_days_of_childbirth() const;

  // useful functions?
  int calculate_age(int current_day) const; // in years
  bool is_eligible(int current_day) const; // adult and un-partnered
  bool carries_h_influenzae() const;

  // colonization stuff
  std::vector<double> get_susceptibility() const; // calculate them when needed (which is every time step?) implementation detail
  int get_susceptibility(int serotype) const; // needed to calculate force of colonization

  std::vector<Colonization>> contract_colonization(int current_day, int serotype); // returns the new colonization (first element of vector)
                                                                                   // and colonizations modified as a result
private:
  // helpers
  void update_susceptibilities(); // everytime there's a change in carriage

private:
  int host_id_ // ??? unique idenfier, like a name

  // day_of_[major life events]
  int day_of_birth_;
  int day_of_death_;
  int day_of_fledge_;
  int day_of_partnering_;
  std::vector<int> days_of_childbirth_;

  // colonization details
  std::vector<double> susceptibilities_;  // indexed by strain, values in [0, 1]
  std::vector<int> num_past_colonizations_;  // indexed by strain, used to calculate susceptibility and mean duration of colonization
  std::vector<Colonization> current_colonizations_; // maybe use some other container, bool is a "modified" flag
  bool carries_h_influenzae; // will we need to keep track of hflu colonization details?

  // parameters. instantiate in .cpp using utils.h?
  static const int MATURITY_AGE;

  // immunity parameters
  static const int MU; // scales the resistance to acquisition conferred by most fit strain
  static const int SIGMA; // anti-capsular immmunity (serotype-specific immunity)

  static next_unused_id;
  static RandomNumberGenerator& random_number_generator_; // initialize in main.cpp?
};

/* event.h
 * An event is meant to keep track of stuff that will happen in future simulation time-steps. 
 */
class Event {
  enum class Type {
    FLEDGE, PARTNERING, CHILDBIRTH, DEATH, RECOVERY
  };
public:
  LifeEvent(int day_of_event, Type type) : 
    day_of_event_(day_of_event), 
    type_(type) {};
  int get_day_of_event() const;
  Type get_type() const;
private:
  int day_of_event_;
  Type type_;
};


/* event_queue.h
 * Keeps a priority queue of events.
 */
#include "host.h"
#include "event.h"

class EventQueue {
public:
  Event pop();
  int day_of_next_event();
  int schedule_life_events(Host host); // returns number of events added
  int schedule_recovery(Host host, Colonization colonization); // returns number of events added
private:
  // some STL or boost container <Event>
};

/* population.h
 * Container of Hosts. Provides const iterator for read-only access to individual Hosts 
 * member function to delete hosts, and statistics.
 */
#include <set>
#include <vector>
#include "host.h"
#include "random_number_generator.h"

class Population {
  Population(const std::vector<double> initial_age_distribution,
             const std::vector<double> lifespan_distribution,
             const std::vector<double> fledge_age_distribution,
             const std::vector<double> partnering_age_distribution,
             const std::vector<double> childbirth_age_distribution,
             const std::vector<double> parity_distribution) :
    initial_age_distribution_(initial_age_distribution);
    lifespan_distribution_(lifespan_distribution),
    fledge_age_distribution_(fledge_age_distribution),
    partnering_age_distribution_(partnering_age_distribution),
    childbirth_age_distribution_(childbirth_age_distribution),
    parity_distribution_(parity_distribution);

  int initialize(int num_hosts); // create initial population

  int get_size() const; // number of hosts
  int get_number_colonizations(int serotype) const; // a host infected with two strains of the same 
                                                 // serotype is counted twice
  int get_number_colonizations_by_age(int serotype, int age_in_years) const;

  Host birth_new_host(int parent_host_id, int current_day); // returns newborn host
  void fledge_host(int host_id);
  void partner_host(int host_id);
  void delete_host(int host_id); 

  std::vector<Host> colonize_hosts(int serotype, int current_day); // returns hosts with new colonizations
  void recover_host(int serotype, int strain);

  // ??? std::vector<int> get_age_frequencies; // frequency of each age (years)
  // ??? std::vector<int> get_household_size_frequencies; // frequency of each household size

  // typedef typename std::set<Host>::const_iterator const_iterator; // exposes implementation detail, but simple.
  // const_iterator begin();
  // const_iterator end();

private:
  // some STL or boost container for hosts
  std::set<Host> hosts_;

  // distributions that characterize a population (used when generating new hosts)
  std::vector<double> initial_age_distribution_;
  std::vector<double> lifespan_distribution_;
  std::vector<double> fledge_age_distribution_;
  std::vector<double> partnering_age_distribution_;
  std::vector<double> childbirth_age_distribution_;
  std::vector<double> parity_distribution_;

  std::vector<int> number_of_colonizations_; // indexed by serotype

  // parameters that characterize transmission
  static const double CONTACT_RATE; // contact rate shared by all serotypes
  static const double IMMIGRATION_RATE; // low immigration rate of each serotype (shared by all serotypes?)
  static const std::vector<std::vector<double>> AGE_WAIFW_MATRIX; // for age-assortative mixing
  static const double RHO; // fraction of force of infection from household

  static RandomNumberGenerator& random_number_generator_; // initialize in main.cpp?
};

/* simulation.h
 * setup, runs simulation, generates snapshots
 */
#include <vector>
#include "population.h"
#include "life_event_queue.h"
#include "recovery_event_queue.h"
#include "event_scheduler.h"
#include "host.h"
#include "life_event.h"
#include "recovery_event.h"

class Simulation {
  Simulation();

  void setup();
  /*
    instantiate objects,
    initialize population
    schedule all life events 
  */
  void run_one_day(bool demographics_only=false);
  /*
    1. process all events up to current day
      a. may include new births
      b. may end up deleting hosts
    2. if (not demogrpahics_only) 
      a. tell population to colonize hosts
      b. schedule new events due to colonization
    3. schedule life events of new births
  */

  int get_current_day() const;

  struct Snapshot {
    std::vector<int> age_frequencies;
    std::vector< std::vector<int> > colonization_frequencies_by_age;
    // and more!
  };
  Snapshot get_snapshot();

private:
  // helpers to process different events
  void process_life_event(LifeEvent life_event);
  void process_recovery_event(LifeEvent recovery_event);

  int current_day_; // initialize to 0
  Population population_; // create one based on configuration details (different age distributions)
  LifeEventQueue life_event_queue_; // initialize to empty
  RecoveryEventQueue recovery_event_queue_; // initilize to empty
  EventScheduler event_scheduler_; // create one
};

/*
main.cpp

initialize RNG and set it to be Population::RNG, Host::RNG, and Colonization::RNG 
open files
run simulations to determine beta
run real simulation
  write snapshots to files
close files
*/

/*
possible command line usage

./pneumo-abm 
  --output <directory name> 
  --config <configuration directory> 
  [--seed <some integer>] 
  [--match_prevalence <prevalence file>]

./pneumo-abm 
  -o ../../outputs/trial-run 
  -c ../../configuration 
  -s 0
  -m ../../configuration/observed_prevalences.json

./pneumo-abm run --config <path> --num_trials <arg>
./pneumo-abm match

*/
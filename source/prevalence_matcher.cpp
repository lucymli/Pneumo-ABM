#include <algorithm>
#include <cmath>
#include <ctime>
#include <stdexcept>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

#include "configuration.h"
#include "prevalence_matcher.h"
#include "simulation.h"
#include "vaccine.h"

namespace fs = boost::filesystem;

// mathematical helper functions
namespace {
  double log_factorial(unsigned int n) {
  // Ramanujan approximation
  return n * log(n) - n 
    + log(1.0 + 1.0 / (2.0 * n) + 1.0 / (8.0 * n * n)) / 6.0 
    + log(2.0 * n) / 2.0
    + log(3.141592653589793238462643383) / 2.0;
  } 

  double multinomial_loglikelihood(std::vector<double> p, std::vector<unsigned int> x) {
    int n = std::accumulate(x.begin(), x.end(), 0);
    double llh = log_factorial(n);
    for (int i = 0; i < p.size(); i++) {
      llh -= log_factorial(x[i]);
      llh += x[i] * log(p[i]);
    }
    return llh;
  }

  double clamp(double x, double min, double max) {
    return std::min(std::max(x, min), max);
  }
}

PrevalenceMatcher::PrevalenceMatcher(PrevalenceMatcher::Mode mode, int seed, std::string config_file, std::string output_folder) 
  : mode_            { mode }
  , seed_            { seed } 
  , config_file_     { config_file }
  , output_folder_   { output_folder }
  , iteration_       { 0 }
  , previous_prevalence_errors_ { std::vector<double>() } // empty
  , start_time_      { std::time(nullptr) }
  , scribe_          { output_folder }
{
  // assign instance variables based on configuration file
  configure(config_file);

  // save configuration details
  fs::path source = (fs::path {config_file}).parent_path();
  scribe_.copy_folder(source.string());
  scribe_.note_value(seed_, "seed.txt");
  scribe_.note_values(target_prevalences_, "target_prevalences.csv");
}

void PrevalenceMatcher::configure(std::string config_file) {
  namespace fs = boost::filesystem;
  namespace pt = boost::property_tree;

  pt::ptree ptree = load_config(config_file, "fitting");
  max_iterations_        = ptree.get<unsigned int>("max_iterations");
  max_samples_           = ptree.get<unsigned int>("max_samples");
  warm_up_               = ptree.get<double>("warm_up");
  cool_down_             = ptree.get<double>("cool_down");
  temperature_threshold_ = ptree.get<double>("temperature_threshold");

  auto config_folder = fs::path(config_file).parent_path();
  auto counts_path = (config_folder / ptree.get<std::string>("observed_counts_file")).string();
  observed_counts_ = load_vector<unsigned int>(counts_path, "counts");
  if ((observed_counts_.size()%(Serotype::get_num_serotypes() + 1)) != 0) {
    throw std::runtime_error {"Number of observed counts does not match number of serotypes plus one (for un-colonized)."};
  }
  target_prevalences_ = calculate_prevalences(observed_counts_);
  data_years_ = as_vector<int>(ptree.get_child("data_years")); // compare prevalences at these number of years after simulation starts
  
  //switch (mode_) {
    //case PrevalenceMatcher::Mode::FitTransmission: {
      beta_            = ptree.get<double>("initial_values.beta");
      beta_weight_     = ptree.get<double>("initial_weights.beta");

      auto ranks_path = (config_folder / ptree.get<std::string>("initial_values.ranks_file")).string();
      ranks_ = load_vector<double>(ranks_path, "ranks");
      if (ranks_.size() != Serotype::get_num_serotypes()) {
       throw std::runtime_error {"Number of initial ranks does not match number of serotypes."};
      }
      double initial_rank_weight = ptree.get<double>("initial_weights.rank");
      rank_weights_ = std::vector<double>(Serotype::get_num_serotypes(), initial_rank_weight);
//      break;
//    } 
//    case PrevalenceMatcher::Mode::FitVaccine: {
      vaccine_to_fit_                      = ptree.get<std::string>("vaccine_to_fit");
      max_susceptibility_reduction_        = ptree.get<double>("initial_values.max_susceptibility_reduction");
      max_susceptibility_reduction_weight_ = ptree.get<double>("initial_weights.max_susceptibility_reduction");
//      break;
//    }
//  }
}

void PrevalenceMatcher::match() {
  auto start_time = std::time(nullptr);

  // set the relevant initial values
//  if (mode_ == PrevalenceMatcher::Mode::FitTransmission) {
    std::cout << "Starting fitting process for transmission parameters..." << std::endl;
//  } else if (mode_ == PrevalenceMatcher::Mode::FitVaccine) {
    std::cout << "Starting fitting process for " << vaccine_to_fit_ << " parameters..." << std::endl;
//  }

  for (; iteration_ < max_iterations_; iteration_++) {
    // create a simulation 
    std::string output_folder_i = output_folder_ + "/iteration-" + std::to_string(iteration_);
    Simulation simulation { seed_, config_file_, output_folder_i };
    
    // update parameters
//    if (mode_ == PrevalenceMatcher::Mode::FitTransmission) {
      simulation.set_population_beta(beta_);
      Serotype::set_ranks(ranks_);
//    } else if (mode_ == PrevalenceMatcher::Mode::FitVaccine) {
      Vaccine::set_max_reduction(vaccine_to_fit_, Vaccine::EffectType::Susceptibility, max_susceptibility_reduction_);
//    }

    // run simulation
    auto snapshots = simulation.run();
    estimated_prevalences_ = calculate_prevalences(snapshots, max_samples_);

    // calculate serotype-specific prevalence errors
    prevalence_errors_ = calculate_errors(estimated_prevalences_, target_prevalences_);
    
    loglikelihood_ = multinomial_loglikelihood(estimated_prevalences_, observed_counts_);

    // write to file how that simulation went
    save_snapshot();

    // adjust the relevant parameters
//    if (mode_ == PrevalenceMatcher::Mode::FitTransmission) {
      if ((iteration_ % 2)==1) update_transmission_parameters();
//    } else if (mode_ == PrevalenceMatcher::Mode::FitVaccine) {
      else update_vaccine_parameters();
//    }
    previous_prevalence_errors_ = prevalence_errors_;
    std::cout << "Iteration " << iteration_ + 1 << " of " << max_iterations_ << " completed." << std::endl;
  }
  auto num_secs = std::time(nullptr) - start_time;
  std::cout << "Fitting complete. Took " << num_secs / 60 << " minutes and " << num_secs % 60 << " seconds." << std::endl;
}

double PrevalenceMatcher::sum_errors (std::vector <double> & errors, int num_serotypes) {
  double total_error = 0.0;
  int num_time_points = errors.size() / (num_serotypes+1);
  for (int i=0; i<errors.size(); i++) {
    if ((i%num_serotypes)!=0) {
      total_error += errors[i];
    }
  }
  return (total_error/(double) num_time_points);
}

void PrevalenceMatcher::update_transmission_parameters() {
  // update beta:
  // 1. calculate error (sum of prevalence errors)
  int num_serotypes = ranks_.size();
  int num_time_points = prevalence_errors_.size() / (num_serotypes+1);
  double error = sum_errors(prevalence_errors_, num_serotypes);
  
  // 2. adjust weight if we can compare this error to the error from the previous iteration
  if (!previous_prevalence_errors_.empty()) {
    double target = sum_errors(target_prevalences_, num_serotypes);
    double previous_error = sum_errors(previous_prevalence_errors_, num_serotypes);
    adjust_weight(beta_weight_, target, error, previous_error);
  }
  
  // 3. adjust the parameter
  adjust_beta(beta_, beta_weight_, error);
  if (beta_<0) beta_ *= -1.0;
  if (beta_weight_<0) beta_weight_ *= -1.0;
  // update ranks:
  if (!previous_prevalence_errors_.empty()) {
    adjust_weights(rank_weights_, target_prevalences_, prevalence_errors_, previous_prevalence_errors_);
  }
  adjust_ranks(ranks_, rank_weights_, prevalence_errors_);
  
  // go back and constrain the rank of the fittest strain to be 1
  auto fittest_itr = std::max_element(begin(target_prevalences_), end(target_prevalences_) - 1);
  auto fittest_index = std::distance(begin(target_prevalences_), fittest_itr);
  ranks_[fittest_index] = 1;
}

void PrevalenceMatcher::update_vaccine_parameters() {
  // update maximum susceptibility reduction of vaccine
  // helpful lambda
  auto sum_over_vaccine_types = [this](std::vector<double> serotype_values) {
    double sum { 0 };
    for (int i = 0; i < Serotype::get_num_serotypes(); i++) {
      if (Vaccine::targets_serotype(this->vaccine_to_fit_, Vaccine::EffectType::Susceptibility, Serotype::get_name(i))) {
        sum += serotype_values[i];
      }
    }
    return sum;
  };
  // 1. calculate error (sum of prevalence errors of vaccine serotypes)
  double error = sum_over_vaccine_types(prevalence_errors_);
  // 2. adjust weight 
  if (!previous_prevalence_errors_.empty()) {
    double previous_error = sum_over_vaccine_types(previous_prevalence_errors_);
    double target = sum_over_vaccine_types(target_prevalences_);
    adjust_weight(max_susceptibility_reduction_weight_, target, error, previous_error);
  }
  // 3. update parameter
  adjust_max_susc_reduction(max_susceptibility_reduction_, max_susceptibility_reduction_weight_, error);
}


std::vector<double> PrevalenceMatcher::calculate_prevalences(std::vector<unsigned int> counts) {
  std::vector<double> prevalences;
  double total = (double) std::accumulate(counts.begin(), counts.end(), 0);
  for (auto c : counts)
    prevalences.push_back(c / total);
  return prevalences;
}

std::vector<double> PrevalenceMatcher::calculate_prevalences(std::vector<unsigned int> counts, int num_serotypes) {
  std::vector<double> prevalences;
  int num_time_points = counts.size() / (num_serotypes + 1);
  double total;
  for (int t=0; t<num_time_points; t++) {
    total = (double) std::accumulate(counts.begin()+t*(num_serotypes+1), counts.begin()+(t+1)*(num_serotypes+1), 0);
    for (int i=0; i<num_serotypes+1; i++) {
      prevalences.push_back((double)counts[t*(num_serotypes+1)+i] / total);
    }
  }
  return prevalences;
}

std::vector<double> PrevalenceMatcher::calculate_prevalences(std::vector<Simulation::Snapshot> snapshots, unsigned int max_samples) {
  if (snapshots.empty()) {
    throw std::runtime_error { "PrevalenceMatcher::calculate_prevalence: snapshots vector is empty." };
  }

  int num_serotypes = snapshots[0].num_colonized_under_5.size();
  int n = (int) snapshots.size();
  int num_samples = std::min((int) max_samples, n);
  auto prevalences = std::vector<double>(num_samples*(num_serotypes+1), 0);
  //for (int i = n - 1; i >= n - num_samples; i--) { // if max_samples_==1, this loop takes the most recent
  for (int i=0; i<num_samples; i++) {
    for (int s = 0; s < num_serotypes; s++) {
      prevalences[i*(num_serotypes+1)+s] += (1.0 / num_samples) * (snapshots[data_years_[i]].num_colonized_under_5[s] / (double) snapshots[data_years_[i]].num_hosts_under_5);
    }
    // add prevalence of un-colonized
    double colonized_prevalence = std::accumulate(prevalences.begin()+i*(num_serotypes+1), prevalences.begin()+(i+1)*num_serotypes, 0.0);
    prevalences[(i+1)*(num_serotypes+1)-1] = 1.0 - colonized_prevalence;
  }
  return prevalences;
}

std::vector<double> PrevalenceMatcher::calculate_errors(std::vector<double> estimate, std::vector<double> target) {
  if (estimate.size() != target.size())
    throw std::runtime_error {"PrevalenceMatcher::calculate_errors: Expected two vectors of the same length."};
  auto errors = std::vector<double>(target.size(), 0.0);
  for (int i = 0; i < estimate.size(); i++) {
    errors[i] = estimate[i] - target[i];
  }
  return errors;
}

void PrevalenceMatcher::adjust_weight(double &weight, const double target, const double error, const double previous_error) {
  if (previous_error * error < 0 && fabs(error) > fabs(previous_error)) { 
    weight *= cool_down_; // overshot, went too far the other way
  } else if (fabs(error) / fabs(previous_error) > temperature_threshold_) {
    weight *= warm_up_;   // progressing too slowly
  } else {
    weight *= cool_down_; // getting close
  }
}

void PrevalenceMatcher::adjust_weights(std::vector<double>& weights, const std::vector<double>& targets, 
                                       const std::vector<double>& errors, const std::vector<double>& previous_errors) {
  int num_serotypes = weights.size();
  int num_time_points = targets.size() / (num_serotypes+1);
  for (int t=0; t<num_time_points; t++) {
    for(int i = 0; i < weights.size(); i++) {
      int j = t*(num_serotypes+1)+i;
      adjust_weight(weights[i], targets[j], errors[j], previous_errors[j]);
    }
  }
}

void PrevalenceMatcher::adjust_beta(double &beta, const double weight, const double error) {
  beta *= (1.0 - weight * error);
}

void PrevalenceMatcher::adjust_max_susc_reduction(double &max_susceptibility_reduction, const double weight, const double error) {
  max_susceptibility_reduction *= (1.0 + weight * error); // note: positive error (too much vaccine types) -> increase vaccine efficacy
  max_susceptibility_reduction = clamp(max_susceptibility_reduction, 0, 1);
}

void PrevalenceMatcher::adjust_ranks(std::vector<double> &ranks, 
                                     const std::vector<double>& weights,
                                     const std::vector<double>& errors) {
  //int arr[10] = {4, 10, 12, 18, 31, 32, 33, 38, 39, 40};
  int num_serotypes = ranks.size();
  int num_time_points = errors.size() / (num_serotypes + 1);
  for(int i = 0; i < ranks.size(); i++) {
    bool next = false;
    //    for (int j=0; j<10; j++) {
    //      if (i==arr[j]) next = true;
    //    }
    if (!next) {
      double error = errors[i];
      if (num_time_points > 1) {
        for (int t=1; t<num_time_points; t++) {
          error += errors[t*(num_serotypes+1)+i];
        }
      }
      error /= (double) num_time_points;
      ranks[i] *= (1.0 + weights[i] * error);
      ranks[i] = clamp(ranks[i], Serotype::get_min_rank(), Serotype::get_max_rank());
    }
  }
}


void PrevalenceMatcher::save_snapshot() {
  scribe_.note_value (iteration_,             "iteration.csv");
  scribe_.note_values(estimated_prevalences_, "estimated_prevalences.csv");
  scribe_.note_values(prevalence_errors_,     "prevalence_errors.csv");
  scribe_.note_value (loglikelihood_,         "loglikelihood.csv");

  //if (mode_ == PrevalenceMatcher::Mode::FitTransmission) {
    scribe_.note_value (beta_,  "beta.csv");
    scribe_.note_values(ranks_, "ranks.csv");
  //} else if (mode_ == PrevalenceMatcher::Mode::FitVaccine) {
    scribe_.note_value(max_susceptibility_reduction_, "max_susceptibility_reduction.csv");
  //}
}

from bunch import Bunch
import json
import math
import os
import pneumodel
import sys
import time

##################################
###  Functions used in fitting ###
##################################

##################
# Loss functions #
##################

def _log_factorial(n):
  ''' log(n!) using Ramanujan's approximation '''
  assert n > 0
  return (
    n * math.log(n) - n +
          math.log(1.0 + 1.0 / (2.0 * n) + 1.0 / (8.0 * n * n)) / 6.0 +
          math.log(2.0 * n) / 2.0 +
          math.log(3.141592653589793238462643383) / 2.0
  )

def _multinomial_loglikelihood(p, x):
  ''' P(x|p) where x ~ Multinomial(p) '''
  assert len(p) == len(x)
  for p_i in p:
    assert 0.0 <= p_i <= 1
  for x_i in x:
    assert x >= 0
  n = sum(x)
  llh = _log_factorial(n)
  for p_i, x_i in zip(p, x):
    llh += (-_log_factorial(x_i) + x_i * math.log(p_i))
  return llh


def total_prevalence_absolute_loss(config_file, theta, output_dir):
  # set point by updating config file
  _update_config(config_file, theta)
  # get results
  pneumodel.run_simulation(config_file, output_dir)
  results = pneumodel.get_simulation_results(output_dir)
  ...

#########################
# Pertubation functions #
#########################

 

######################
# Boundary functions #
######################

def standard_boundary(theta):
  # beta must be non-negative
  if 'beta' in theta:
    theta.beta = max(0, theta.beta)

  # ranks must be in [1, num_serotypes]
  num_serotypes = len(theta.ranks)
  if 'ranks' in theta:
    for i, x in enumerate(theta.ranks):
      theta.ranks[i] = max(1, min(x, num_serotypes))

  # vaccine efficacies must be in [0, 1]
  if 'efficacies' in theta:
    for i, x in enumerate(theta.efficacies):
      theta.efficacies[i] = max(0, min(x, 1))

  return theta


########################
###  Fitting routine ###
########################

def fit_model(config_file, output_dir, theta, loss_func, perturb_func, update_func, boundary_func):
  ''' Fits model parameters.
  
  Expects a configuration file that specifies:
  - num_iterations        
  - simulation_config_file
  theta       : starting point in parameter space
  Points should be represented as an object with the following attributes:
    theta.beta
    theta.ranks   : ranks of the serotypes
    theta.efficacies : (vaccine_name, efficacy) pairs

  The other arguments are functions. (i is iteration number)
  loss     : point, config, output_dir -> loss (scale- or vector-valued)
  perturb  : i, current point -> a set of new points
  update   : i, set of (p, loss(p)) pairs -> proposed next point
  boundary : proposed next point -> next point satisfying boundary conditions
  
  '''
  try:
    os.makedirs(output_dir)
  except os.error as e:
    raise ValueError('Error creating {}: {}'.format(output_dir, e))

  get_output_dir = lambda i, j: os.path.join('iter-{}'.format(i), 'point-{}'.format(j))

  print ('Starting fitting process...')
  history = []

  # make a working copy of the configuration files
  working_config_file = pneumodel.copy_config_files(config_file, os.path.join(output_dir, 'working-configuration'))
  
  # check loss at the start
  history.append(theta, loss_func(working_config_file, theta, get_output_dir(0, 0)))

  # iterations of the fitting process
  for i in range(n_iters):
    t0_iter = time.time()
    print ('Iteration {} of {}...'.format(i + 1, n_iters),)

    # perturb points
    perturbed = perturb_func(i, theta)

    # calculate losses at perturbed points
    losses = []
    for j, p in enumerate(perturbed):
      print ('\tCalculating loss for {} of {} points...'.format(j, len(perturbed)),
      losses.append(p, loss(working_config_file, p), get_output_dir(i + 1, j)))

    # propose the next point, apply boundary conditions
    proposal = update_func(i, losses)
    theta = boundary_func(proposal)

    # check our loss now
    history.append(theta, loss_func(working_config_file, theta, get_output_dir(0, 0)))

    # display time elapsed for this iteration
    t_iter = time.time() - t0_iter
    print ('completed. {}m {}s'.format(int(t_iter / 60), t_iter % 60))





from bunch import Bunch
import json
import numpy as np
import pandas as pd
import os
import shutil
import subprocess
import time

EXEC_PATH = '/Users/ocsicnarf/Github/Pneumo-ABM/DerivedData/PneumoModel/Build/Products/Release/pneumo-model'

def _load_json(path):
  with open(path) as f:
    return json.load(f, object_hook=lambda d: Bunch(d))

def _load_csv(path):
  ''' Load a csv as a numpy array. '''
  return np.array(pd.read_csv(path, header=None))

def _get_referenced(config):
  filenames = []
  for k, v in config.iteritems():
    if isinstance(v, Bunch):
      filenames += _find_filenames(v)
    elif k.endswith('file'):
      filenames.append(v)
    return filenames

def _find_index(lst, cond):
  return next((i for i, v in enumerate(lst) if cond), -1)

def copy_config_files(config_file, dst_dir):
  ''' Copies a configuration file and the files it references. '''

  try:
    os.makedirs(dst_dir)
  except os.error as e:
    pass

  # copy the configuration file, renamed as 'configuration.json'
  config_basename, config_dir = os.path.split(config_file)  
  dst_config_file = os.path.join(dst_dir, 'configuration.json')
  shutil.copyfile(config_file, dst_config_file)

  # copy all the files it references
  config = _load_json(config_file)
  for filename in _get_referenced(config):
    shutil.copyfile(
      os.path.join(config_dir, filename),
      os.path.join(dst_dir,   filename)
    )

  return dst_config_file


def update_config_file(config_file, theta):
  ''' Updates a configuration file according to values found in theta (a Bunch) '''
  
  with open(config_file, 'r') as f:
    config = json.load(f, object_hook=lambda d: Bunch(d))
  
  # update beta
  if 'beta' in theta:
    config.population.beta = theta.beta

  # update ranks (first check new ranks have same dimension)
  if 'ranks' in theta:
    ranks_file = os.path.join(os.path.dirname(config_file), config.serotype.ranks_file)
    with open(ranks_file, 'r') as f:
      ranks = json.load(f)['ranks']
      assert len(ranks) == len(theta.ranks)
    with open(ranks_file, 'w') as f:
      json.dump({'ranks': theta.ranks}, f)

  # update vaccine efficacies
  if 'efficacies' in theta:
    for name, efficacy in theta.efficacies:
      i = _find_index(config.vaccines, lambda v: v.name == name)
      if i < 0:
        raise ValueError('Vaccine "{}" not found in configuration file.'.format(name))
      config.vaccines[i].susceptibility_reduction.max = efficacy
    with open(config_file, 'w') as f:
      json.dump(config, f)

def run_simulation(config_file, output_dir, num_trials=1):
  ''' Runs a simulation, returns elapsed time. '''

  t0 = time.time()
  cmd = [EXEC_PATH, '-c', config_file, '-o', output_dir, '-t', 'simulate', '-n', str(num_trials)]
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  return_code = p.wait()
  if return_code != 0:
    for line in p.stderr:
      print(line)
    raise subprocess.CalledProcessError(return_code, cmd)

  return time.time() - t0

def get_simulation_results(output_dir, trial_num=0):
  ''' Processes simulation output files and returns a results object. '''

  config    = _load_json(os.path.join(output_dir, 'configuration', 'configuration.json'))
  serotypes = _load_json(os.path.join(output_dir, 'configuration', config.serotype.serotypes_file)).serotypes
  ranks     = _load_json(os.path.join(output_dir, 'configuration', config.serotype.ranks_file)).ranks

  trial_dir = os.path.join(output_dir, 'trial-{}'.format(trial_num))
  num_hosts = _load_csv(os.path.join(trial_dir, 'num_hosts.csv')).flatten()

  num_years = (
    config.simulation.num_years_burn_in.demographic 
    + config.simulation.num_years_burn_in.epidemiologic
    + config.simulation.num_years_simulation
  )
  max_age = config.population.max_age
  num_serotypes = len(serotypes)

  num_colonized = np.zeros(shape=(num_years + 1, num_serotypes, max_age + 1))
  for i, st in enumerate(serotypes):
    num_colonized[:, i, :] = _load_csv(os.path.join(trial_dir, 'num_colonized_by_age_ss_{}.csv'.format(st)))

  return Bunch(
    config        = config,
    serotypes     = np.array(serotypes),
    ranks         = np.array(ranks),
    num_hosts     = num_hosts,
    num_colonized = num_colonized
  )


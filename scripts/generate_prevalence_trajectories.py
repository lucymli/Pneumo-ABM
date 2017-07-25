#! /usr/bin/env python

import argparse
import csv
import itertools
import math
import os
import pneumodel.trial as pn
import sys
import time

def _parse_args():
  parser = argparse.ArgumentParser(description='Compute the time-varying prevalence in each vaccine trial arm.')
  parser.add_argument('-l', '--logs-folder', required=True, help='Path to a folder containing event logs from one or more simulation runs.')
  parser.add_argument('-d', '--days-between', type=int, required=True, help='Time interval, in days, between prevalence computations.')
  parser.add_argument('-f', '--follow-up-days', type=int, required=True, help='Number of days to follow the trial participants, starting from birth.')

  return parser.parse_args()

def _get_num_runs(logs_folder):
  n = 0
  while(True):
    path = os.path.join(logs_folder, 'trial-{}'.format(n))
    if not os.path.exists(path):
      break
    else:
      n += 1
  return n

def _elapsed(start_time):
  T = time.time() - start_time
  return T // 60, T % 60

def _transpose_csv(path, out):
  with open(path, 'r') as f:
    zipped = itertools.izip(*csv.reader(f))
  with open(out, 'w') as f:
    csv.writer(f).writerows(zipped)

if __name__ == '__main__':
  t0 = time.time()

  # get arguments from command line
  args = _parse_args()
  num_samples = int(math.ceil(args.follow_up_days / float(args.days_between))) + 1
  days_since_birth = [k * args.days_between for k in range(num_samples)]

  print('---Generate Prevalence Trajectories---')
  print('Event logs from {}'.format(os.path.abspath(args.logs_folder)))
  print('Compute prevalences every {} days until at least {} days after birth.'.format(args.days_between, args.follow_up_days))

  # create directory to store csv files
  dest_dir = os.path.join(args.logs_folder, 'trial-prevalences-{}-{}'.format(args.days_between, args.follow_up_days))
  try:
    os.mkdir(dest_dir)
  except OSError:
    raise RuntimeError('Trying to create {}, but it already exists'.format(dest_dir))
  print('Prevalences will be written to {}'.format(os.path.abspath(dest_dir)))
  
  # create a csv file for each trial arm
  trial_0 = pn.Trial(args.logs_folder, 0)
  arm_names_0 = trial_0.arms.keys()
  for a in arm_names_0:
    with open(os.path.join(dest_dir, '{}.csv.temp'.format(a)), 'w') as f:
      D = max(trial_0.arms[a].config.schedule)
      days_since_vac = [d - D for d in days_since_birth] # days since vaccination
      w = csv.writer(f, quoting=csv.QUOTE_NONE)
      w.writerow(['dsb'] + days_since_birth)
      w.writerow(['dsv'] + days_since_vac)

  # process each run
  num_runs = _get_num_runs(args.logs_folder)
  for i in range(num_runs):
    t00 = time.time()
    sys.stdout.write('Processing run {} of {}'.format(i + 1, num_runs))
    sys.stdout.flush()
  
    trial = pn.Trial(args.logs_folder, i)
    arm_names = trial.arms.keys()
    assert(sorted(arm_names) == sorted(arm_names_0))
    for a in arm_names:
      sys.stdout.write('.')
      sys.stdout.flush()

      xs = pn.get_prevalence_trajectory(trial.arms[a], days_since_birth, first_day=0)
      with open(os.path.join(dest_dir, '{}.csv.temp'.format(a)), 'a') as f:
        writer = csv.writer(f, quoting=csv.QUOTE_NONE)
        writer.writerow(['run-{}'.format(i)] + ['{:.5f}'.format(x) for x in xs])

    sys.stdout.write('{:.0f}m{:.1f}s\n'.format(*_elapsed(t00)))
    sys.stdout.flush()

  # transpose the csv.temp files
  for filename in os.listdir(dest_dir):
    if filename.endswith('.csv.temp'):
      filepath = os.path.join(dest_dir, filename)
      out = os.path.join(dest_dir, '{}.csv'.format(filename.split('.')[0]))
      _transpose_csv(filepath, out)
      os.remove(filepath)

  sys.stdout.write('{:.0f}m{:.1f}s\n'.format(*_elapsed(t0)))
  sys.stdout.flush()

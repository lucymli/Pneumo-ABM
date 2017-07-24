import json
import os
import numpy as np
import pandas as pd
import re
from statsmodels.stats import proportion

# Working with files and output directories
def read_json(key, *path_components):
  with open(os.path.join(*path_components)) as f:
    return np.array(json.load(f)[key])

def read_csv(*path_components):
  return np.array(pd.read_csv(os.path.join(*path_components), header=None))

def get_num_trials(output_dir):
  ''' Assumes that output_dir has folders named 'trial-0', 'trial-1', etc. '''
  n = 0
  while os.path.exists(os.path.join(output_dir, 'trial-{}'.format(n))):
    n += 1
  return n

# 
def get_categorizer(vaccine_types):
  extract_serogroup = lambda s: re.search('(\d+)\D*', s).group(1)
  vaccine_groups = set(extract_serogroup(s) for s in vaccine_types)
  def categorizer(s):
    if s in vaccine_types:
      return 'VT'
    elif extract_serogroup(s) in vaccine_groups:
      return 'VRT'
    else:
      return 'NVT'
  return np.vectorize(categorizer)

# Retrieving data
def get_observed_data(path, index, column_suffix='', categorizer=lambda x: x):
  # read in observed counts
  counts = read_json('counts', path)
  num_colonized, num_samples = counts[:-1], counts.sum()

  # group by category
  df_counts = pd.DataFrame(dict(num_colonized=num_colonized), index=index)
  df_counts['category'] = categorizer(df_counts.index)
  df = df_counts.groupby('category', as_index=True).sum()

  # calculate prevalence and add to dataframe
  prevalences = df.num_colonized / float(num_samples)
  confint = proportion.proportion_confint(df.num_colonized, num_samples, alpha=0.05, method='beta')
  df['prev_point'] = prevalences
  df['prev_lower'] = np.nan_to_num(confint[0])
  df['prev_upper'] = np.nan_to_num(confint[1])

  # rename columns
  df.columns = ['{}_{}'.format(c, column_suffix) for c in df.columns]
  return df

def get_simulated_data(output_dir, serotypes, years=[], column_suffix=''):
  # create dataframe to hold serotype-specific prevalences at various years
  # each entry (row=serotype, column=year) is a list of prevalences for each trial
  df = pd.DataFrame(
    { yr: [np.array([]) for i in range(len(serotypes))] for yr in years },
    index=serotypes
  )

  # go through each trial
  for i in range(get_num_trials(output_dir)):
    trial_dir = os.path.join(output_dir, 'trial-{}'.format(i))
    num_hosts_by_age = read_csv(trial_dir, 'num_hosts_by_age.csv')

    # for each serotype, record the prevalence at the specified years, for that trial
    for st in serotypes:
      num_colonized = read_csv(trial_dir, 'num_colonized_by_age_ss_{}.csv'.format(st))
      p = num_colonized[:,0:5].sum(axis=1) / num_hosts_by_age[:, 0:5].sum(axis=1)
      for yr in years:
        df.loc[st, yr] = np.append(df.loc[st, yr], p[yr])
  
  # rename columns 
  df.columns = ['prev_{}_{}'.format(c, column_suffix) for c in df.columns]
  return df

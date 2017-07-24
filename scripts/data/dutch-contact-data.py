# Using Data on Social Contacts to Estimate Age-specific Transmission Parameters for Respiratory-spread Infectious Agents, Wallinga et al.

# APPENDIX TABLE 2.
# "Normalized age-specific contact rates cij as estimated from self-reported data # for a typical week, after correction for reciprocity, Utrecht, the Netherlands, # 1986*"

import numpy as np
import json

# age groups: 1–5 6–12  13–19 20–39 40–59 ≥60"

matrix = np.array([
 [169.14,  31.47,  17.76, 34.50, 15.83, 11.47],
 [ 31.47, 274.51,  32.31, 34.86, 20.61, 11.50],
 [ 17.76,  32.31, 224.25, 50.75, 37.52, 14.96],
 [ 34.50,  34.86,  50.75, 75.66, 49.45, 25.08],
 [ 15.83,  20.61,  37.52, 49.45, 61.26, 32.99],
 [ 11.47,  11.50,  14.96, 25.08, 32.99, 54.23]
])

matrix_normed = matrix / np.max(matrix)
json.dumps(np.round(matrix_normed, 4).tolist())

'''
[
  [0.6162, 0.1146, 0.0647, 0.1257, 0.0577, 0.0418],
  [0.1146, 1.0000, 0.1177, 0.1270, 0.0751, 0.0419],
  [0.0647, 0.1177, 0.8169, 0.1849, 0.1367, 0.0545],
  [0.1257, 0.1270, 0.1849, 0.2756, 0.1801, 0.0914],
  [0.0577, 0.0751, 0.1367, 0.1801, 0.2232, 0.1202],
  [0.0418, 0.0419, 0.0545, 0.0914, 0.1202, 0.1976]
]

'''
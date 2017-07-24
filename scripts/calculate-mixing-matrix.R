library(jsonlite)
setwd("~/Github/Pneumo-ABM/scripts")

# These data provided by Stefan Flasche by email February 2016.

# Load population data from Kilifi DSS
pop <- read.csv("data/kenya/age-specific-mixing-kilifi/pop_estimates.csv", row.names=1, header=T)
colnames(pop) <- paste("mid", 2000:2012)

# Get age group sizes from November 2009, when mixing study was conducted
pop.2009 <- pop[,"mid 2009"]
names(pop.2009) <- rownames(pop)

# Load matrix with average number of contacts per day (as reported by row age group)
avg.contacts <- read.csv("data/kenya/age-specific-mixing-kilifi/AvNumberOfContacts_Kenya.csv", row.names=1, header=T)
rownames(avg.contacts) <- rownames(pop)
colnames(avg.contacts) <- rownames(pop)

# Multiply by group size of contacter to get total number of contacts per day (as reported by row age group)
# This should be more symmetric than the average contacts matrix, since we no longer divide by group size
total.contacts <- avg.contacts * pop.2009

# Make it symmetric by taking the average of corresponding off-diagonal elements
total.contacts.sym <- (total.contacts + t(total.contacts)) / 2

# Divide by product of contacter group size and contacted group size
mixing.mat <- total.contacts.sym / (outer(pop.2009, pop.2009))
rownames(mixing.mat) <- rownames(pop)
colnames(mixing.mat) <- rownames(pop)

# Convert to JSON array
mixing.mat.scaled = data.matrix(mixing.mat) / max(mixing.mat)
toJSON(mixing.mat.scaled)
mixing.mat.scaled

library(dplyr)
library(magrittr)
library(ggplot2)



## Load actual data

dir.names <- c("fit-us-transmission-50iter-10000N", "fit-us-transmission-50iter-5000N", "fit-us-transmission-50iter-1000N", "fit-us-transmission-50iter-500N", "test_fit_transmission")

estim.prev <- lapply(paste0(dir.names, "/trial-0/estimated_prevalences.csv"), read.csv)

estim.prev.df <- data.frame(states=10:49,
                            sapply(estim.prev, function (x) x[10:49, ncol(x)]))



ggplot(reshape2::melt(estim.prev.df, id.vars="states")) + theme_bw() +
  geom_density(aes(x=value, fill=variable), alpha=.4)

comparison <- data.frame(N=c(10000, 5000, 1000, 500, 250),
           var=sapply(estim.prev.df[, -1], var),
           time=c(95, 40, 12, 10, 9))

ggplot(comparison%>%reshape2::melt(id.vars="N")) + theme_bw() + 
  geom_point(aes(x=N, y=value)) +
  geom_smooth(aes(x=N, y=value), method="glm", method.args=list(family="Gamma"), se=FALSE) +
  facet_wrap(~variable, ncol=1, scales='free_y')

# Each 10% increase in time, results in a 20% reduction in variance
summary(lm(var~time, data=comparison%>%mutate(var=var/max(var), time=(time-min(time))/min(time))))

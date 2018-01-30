library(dplyr)
library(magrittr)
library(ggplot2)


## Load actual data

data.env <- new.env()
load("data/united-states/data.RData", data.env)
under.5.prev.data <- data.env$serotype.prev.list[1:5] %>%
    lapply(select, -1) %>%
    lapply(rowSums) %>%
    Reduce("+", .) %>%
    data.frame(year=data.env$serotype.prev.list[[1]]$year,
               prev=.)

data.under.5.N <- filter(data.env$swab.data, age_range %in% levels(age_range)[1:5]) %>%
    group_by(year) %>%
    summarize_at("frequency", sum)

data.positive.under.5 <- filter(data.env$serotype.data, Age_Years %in% levels(Age_Years)[1:5]) %>%
group_by(Collection_Year) %>%
summarize_at("Number_of_Serotypes", sum)

data.prev.under.5 <- mutate(data.under.5.N, prevalence=data.positive.under.5$Number_of_Serotypes/frequency)

## Read in simulations

sim.dir <- "test_US_simulations"

trial.ids <- list.dirs(sim.dir, full.names=FALSE, recursive=FALSE) %>%
    grep("trial-", ., value=TRUE) %>%
    gsub("trial-", "", .) %>%
    as.numeric()


num.hosts.under.5 <- lapply(paste0(sim.dir, "/trial-", trial.ids, "/num_hosts_by_age.csv"), read.csv) %>%
    lapply(select, 1:5) %>%
    lapply(rowSums) %>%
    as.data.frame

simdata <- lapply(paste0(sim.dir, "/trial-", trial.ids, "/num_colonized_under_5.csv"), read.csv) %>%
    as.data.frame() %>%
    `/`(num.hosts.under.5) %>%
    mutate(mean.prev=rowSums(.)/ncol(.)) %>%
    data.frame(year=2011-nrow(.):1, .) %>%
    slice(-1:-100) %>%
    reshape2::melt(id.vars=c("year", "mean.prev"), variable.name="replicate") %>%
    mutate(replicate=as.factor(as.numeric(replicate)))

P <- ggplot(simdata) + theme_bw() +
  stat_summary(aes(x=year, y=value), fun.ymin = min, fun.ymax = max, geom="errorbar") +
  geom_point(aes(x=year, y=value, colour=replicate), alpha=.6) +
  geom_point(data=data.prev.under.5, aes(x=year, y=prevalence), colour="red") +
  scale_colour_manual(values=rep("black", length(levels(simdata$replicate)))) +
  theme(legend.position="none", text=element_text(size=14)) +
xlab("Year") + ylab("Prevalence among children < 5") +
scale_x_continuous(breaks=c(2001, 2003, 2005, 2007, 2009), limits=c(2000, 2010))
ggsave("test_US_sim_prev.pdf", P, width=5, height=3.3)

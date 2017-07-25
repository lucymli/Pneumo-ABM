Exploratory Data Analysis
================
Lucy M Li
7/24/2017

Prevalence of *S. pneumoniae* carriers in the US after PCV7 introduction
------------------------------------------------------------------------

``` r
swab.data <- read.csv("../scripts/data/united-states/SPARC_SwabbingByAge_2001-2014 MDL.csv")
pop.data <- read.csv("../scripts/data/united-states/united-states-pop-pyramid-2001.csv")
serotype.data <- read.csv("../scripts/data/united-states/SPARC2_CollectionYear_Serotypes_Age_2014_BMC.csv")
serotype.data$Serotype[serotype.data$Serotype==""] <- NA
serotype.data$Serotype <- factor(serotype.data$Serotype, exclude=NULL)
serotypes <- levels(serotype.data$Serotype)
data.years <- unique(swab.data$year)
age.groups <- levels(swab.data$age_range)
age.group.labels <- paste0("(", 0:6, ", ", 1:7, "]")
PCV7.serotypes <- c("4", "6B", "9V", "14", "18C", "19F", "23F")
# PCV13.added.serotypes <- c("1", "3", "5", "6A", "7F", "19A")
# PCV13.serotypes <- c(PCV7.serotypes, PCV13.added.serotypes)
VRT.serotypes <- c("6A", "6C", "19A", "9A", "9N", "23A", "23B")
total.vt.vrt.types <- length(c(PCV7.serotypes, VRT.serotypes))
total.types <- length(serotypes)
```

The total number of swabs increased between 2001 and 2014 for all age groups:

``` r
ggplot(swab.data) + theme_bw() +
  geom_line(aes(x=year, y=frequency, colour=age_range)) +
  scale_colour_brewer("Age Range", labels=age.group.labels, palette="Set1") +
  ylab("Number of Swabs") + xlab("Year") +
  theme(text=element_text(size=12))
```

![](01ExploratoryDataAnalysis_files/figure-markdown_github-ascii_identifiers/total_swabs_plot-1.png)

Generate a list of data frames describing the number of swabs in each year for each of the 7 age groups as well as the total number of swabs.

``` r
swab.data.list <- split(swab.data[, -2], swab.data$age_range) %>% add.sum.data.frame
```

``` r
serotype.data.template <- data.frame(year=data.years, matrix(0, nrow=length(data.years), ncol=length(serotypes)))
names(serotype.data.template)[-1] <- serotypes
serotype.data.list <- split(serotype.data[, -4], serotype.data$Age_Years) %>%
  lapply(extract.freq.by.serotype, data.years, serotypes) %>%
  add.sum.data.frame
serotype.prev.list <- mapply(function (a,b) {
  b[, -1] <- sweep(b[, -1], 1, a$frequency, `/`)
  return (b)
}, swab.data.list, serotype.data.list, SIMPLIFY = FALSE)
```

The prevalence of serotypes targetted by the PCV7 vaccine declined after 2000. The prevalence of serotypes 7F and 19A increased in prevalence after PCV7 introduction, but declined after the introduction of PCV13 in 2010.

From 2001 to 2014, 28.1% of non-vaccine types (those not in PCV13) continuously decreased, 28.1% continuously increased, 34.4% increased after PCV7 introduction but decreased after PCV13 introduction, and 9.38% decreased after PCV7 introduction but increased after PCV13 introduction.

Overall, the prevalence of non-vaccine serotypes has increased by 226%.

``` r
serotype.prev.all.long <- summarize.by.serotype(serotype.prev.list$all, PCV7.serotypes, VRT.serotypes)
ggplot(serotype.prev.all.long) + theme_bw() +
  geom_line(aes(x=year, y=frequency, colour=serotype)) +
  geom_vline(xintercept=2010, linetype=2) +
  facet_wrap(~vaccine) +
  scale_colour_manual(values=c("#683567","#66b645","#723ec4","#b5a147","#cf4db7",
                               "#66b390","#d03d56","#5b90b8","#d8723a","#8075cc",
                               "#4b6431","#cf879f","#814330",
                               rep("gray70", total.types-total.vt.vrt.types))) +
  scale_x_continuous(breaks=c(data.years)) +
  xlab("Year") + ylab("Prevalence") +
  guides(colour=guide_legend(ncol=total.vt.vrt.types, byrow=TRUE)) +
  theme(legend.position="bottom", text=element_text(size=14), axis.text=element_text(size=12),
        strip.text=element_text(size=12))
```

![](01ExploratoryDataAnalysis_files/figure-markdown_github-ascii_identifiers/serotype.data.list.visualize-1.png)

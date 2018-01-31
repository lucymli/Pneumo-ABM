find.match <- function (query1, query2, database1, database2, values) {
  select <- which(database1==query1 & database2==query2)
  if (length(select)>0) return (values[select])
  else return (0)
}
extract.freq.by.serotype <- function (data.frame, time.points, types) {
  template <- data.frame(year=time.points, matrix(0, nrow=length(time.points), ncol=length(types)))
  results <- sapply(types, function (sero) {
    sapply(time.points, find.match, sero, data.frame$Collection_Year, 
           data.frame$Serotype, data.frame$Number_of_Serotypes)
  })
  template[, -1] <- results
  names(template)[-1] <- types
  return (template)
}
add.sum.data.frame <- function (list.of.df) {
  list.of.df$all <- data.frame(Reduce('+', list.of.df))
  list.of.df$all[, 1] <- list.of.df[[1]][, 1]
  names(list.of.df$all) <- names(list.of.df[[1]])
  return (list.of.df)
}
summarize.by.serotype <- function (data.frame, PCV7.names, VRT.names) {
  # Convert a wide data frame into a long data frame that describes changes in 
  # frequency as a function of time (year), stratified by serotypes and whether or
  # not the serotype is targeted by vaccines
  data.frame.long <- gather(data.frame, key="serotype", value="frequency", -year)
  data.frame.long$vaccine <- sapply(data.frame.long$serotype, function (x) {
    if (x%in%PCV7.names) return ("PCV7")
    else if (x%in%VRT.names) return ("VRT")
    else return ("NVT")
  })
  other.names <- unique(data.frame.long$serotype)[!(unique(data.frame.long$serotype) %in% c(PCV7.names, VRT.names))]
  data.frame.long$serotype <- factor(data.frame.long$serotype, levels=c(PCV7.names, VRT.names, other.names))
  data.frame.long$vaccine <- factor(data.frame.long$vaccine, levels=c("PCV7", "VRT", "NVT"))
  return (data.frame.long)
}
percentage.increase <- function (vec) {
  scales::percent((vec[2] - vec[1]) / vec[1])
}

plot.serotypes.ts <- function (x, total.vt.vrt.types=14, x.axis.breaks=NULL) {
  require(ggplot2)
  total.types <- length(unique(x$serotype))
  P <- ggplot(x) + theme_bw() +
    geom_line(aes(x=year, y=prevalence, colour=serotype)) +
    geom_vline(xintercept=c(2000, 2010), linetype=2) +
    facet_wrap(~vaccine) +
    scale_colour_manual(values=c("#683567","#66b645","#723ec4","#b5a147","#cf4db7",
                                 "#66b390","#d03d56","#5b90b8","#d8723a","#8075cc",
                                 "#4b6431","#cf879f","#814330",
                                 rep("gray70", total.types-total.vt.vrt.types)))
  if (!is.null(x.axis.breaks)) P <- P + scale_x_continuous(breaks=c(x.axis.breaks))
  P <- P +
    xlab("Year") + ylab("Prevalence") +
    guides(colour=guide_legend(ncol=total.vt.vrt.types, byrow=TRUE)) +
    theme(legend.position="bottom", text=element_text(size=14), axis.text=element_text(size=12),
          strip.text=element_text(size=12))
  return (P)
}

plot.serotypes.boxplot <- function (x, total.vt.vrt.types=14, x.axis.breaks=NULL) {
  require(ggplot2)
  total.types <- length(unique(x$serotype))
  # binomial confidence intervals: https://www.researchgate.net/deref/http%3A%2F%2Fen.wikipedia.org%2Fwiki%2FBinomial_proportion_confidence_interval
  std.errs <- sqrt(x$prevalence * (1-x$prevalence) / x$N)
  x$lower <- sapply(x$prevalence - std.errs, max, 0)
  x$upper <- sapply(x$prevalence + std.errs, min, 1)
  P <- ggplot(x) + theme_bw() + 
    geom_pointrange(aes(x=serotype, y=prevalence, ymin=lower, ymax=upper, colour=factor(year)), position=position_dodge(width=0.3)) +
    # geom_errorbar(aes(x=serotype, ymin=lower, ymax=upper, colour=factor(year)), position="dodge") +
    facet_grid(dataset~vaccine, scales="free_x", drop=TRUE)+
    scale_colour_manual(values=c("#683567","#66b645","#723ec4","#b5a147","#cf4db7",
                                 "#66b390","#d03d56","#5b90b8","#d8723a","#8075cc",
                                 "#4b6431","#cf879f","#814330",
                                 rep("gray70", total.types-total.vt.vrt.types))) +
    xlab("Serotype") + ylab("Prevalence") +
    guides(colour=guide_legend(title="Year", ncol=total.vt.vrt.types, byrow=TRUE)) +
    theme(legend.position="bottom", text=element_text(size=14), axis.text=element_text(size=12),
          strip.text=element_text(size=12))
  return (P)
}
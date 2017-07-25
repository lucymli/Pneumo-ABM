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
summarize.by.serotype <- function (data.frame, PCV7.names, PCV13.names) {
  # Convert a wide data frame into a long data frame that describes changes in 
  # frequency as a function of time (year), stratified by serotypes and whether or
  # not the serotype is targeted by vaccines
  data.frame.long <- gather(data.frame, key="serotype", value="frequency", -year)
  data.frame.long$vaccine <- sapply(data.frame.long$serotype, function (x) {
    if (x%in%PCV7.names) return ("PCV7")
    else if (x%in%PCV13.names) return ("PCV13")
    else return ("NVT")
  })
  return (data.frame.long)
}
percentage.increase <- function (vec) {
  scales::percent((vec[2] - vec[1]) / vec[1])
}
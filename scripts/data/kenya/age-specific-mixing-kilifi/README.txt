These data provided by Stefan Flasche by email 2-2016

See attached the matrix holding the average number of contacts in Kilifi and its conversion into a contact matrix below:


#load population data
#getting age group sizes in Kilifi DSS from November 2009 when mixing was conducted
all_pop=read.csv("Data/pop_estimates.csv",row.names=1,header=T)
colnames(all_pop)=paste("mid",2000:2012)
real_pop=all_pop[,"mid 2009"]
names(real_pop)=c("<1y", "1-5y","6-14y","15-20y","21-49y","50+")

#load matrix with average number of contacts and convert to probability for contact - matrix based on studied population
M=read.csv("Data/AvNumberOfContacts_Kenya.csv",row.names=1,header=T)
M=M*real_pop; M=(M+t(M))/2; M=M/real_pop; Mixing_mat=t(t(M)/real_pop)
rownames(Mixing_mat)=c("<1y", "1-5y","6-14y","15-20y","21-49y","50+")
colnames(Mixing_mat)=rownames(Mixing_mat)


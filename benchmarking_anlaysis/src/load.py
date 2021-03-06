import os, numpy as np, re

def get_np_cpu_walltime(FILE):
	np, CPU, wall 	= None,None,None
	with open(FILE) as FH:
		for line in FH:
			if "-np" in line:

				np=line.split(":")[1]
				np=int(np[1:].strip("\n"))

			elif "CPU" in line:
				info 	= line.split(":")[1]
				ticks, CPU 	= info.split(",")
				CPU 		= float(CPU[1:].strip("\n"))
			elif "Wall" in line:
				wall 	= line.split(":")[1]
				wall 		= float(wall[1:].strip("\n"))
	return np, CPU, wall


def all_files(directory):
	G 	= {}
	for FILE in os.listdir(directory):
		np, CPU, wall 	= get_np_cpu_walltime(directory+FILE)
		if np is not None:
			if np not in G:
				G[np] 	= (list(), list())
			G[np][0].append(CPU)
			G[np][1].append(wall)
	return G
class INFO:
	def __init__(self, H):

		self.models 	= {}
		self.I 			= {}
		self.J 			= {}
		self.L 			= {}
	def insert_data(self, rounds, K, ll , converged, diff, DIR, TYPE2):
		if rounds not in self.models:
			self.models[rounds]={}
		if K not in self.models[rounds]:
			self.models[rounds][K] 	= list()
		self.models[rounds][K].append((ll, converged, diff, DIR, TYPE2))
	def calc_improvements(self, TYPE=1):
		D 		= dict(zip(self.models.keys(), [list() for i in range(len(self.models.keys()))] ))
		
		for rounds in self.models:
			comparisons 	= list()
			for K in self.models[rounds]:
				base_ll 	= np.mean([ll for ll, c, d, t, t2 in self.models[1][K] if t==TYPE])
				comparisons.append(np.mean([ ll-base_ll for ll, converged, diff, ty,t2 in self.models[rounds][K] if ty == TYPE ]) )
			D[rounds] 	= comparisons
		if TYPE==1:
			self.I 	= D
		elif TYPE==2:
			self.J 	= D
	def calc_improvements2(self):
		D 		= dict(zip(self.models.keys(), [list() for i in range(len(self.models.keys()))] ))
		TYPE 	= 2
		for rounds in self.models:
			comparisons 	= {}
			for K in self.models[rounds]:
				for ll, c, d, t,t2 in self.models[rounds][K]:
					base_ll 	= np.mean([LL for LL, c, DD, T, T2 in self.models[1][K] if T==TYPE and t2==T2 ])
					if t 	== TYPE:
						if t2 not in comparisons:
							comparisons[t2]=list()	
						comparisons[t2].append(ll-base_ll)
			D[rounds] 	= comparisons
		self.L 	= D

		

	def get_average_diff(self, TYPE=1): #doesn't take into account number of models
		if TYPE==1:
			return zip(self.I.keys(), [np.mean(c) for c in self.I.values()] )
		return zip(self.J.keys(), [np.mean(c) for c in self.J.values()] )
	def get_average_diff2(self):
		return [ (rounds, [(move, np.mean(self.L[rounds][move])) for move in self.L[rounds] ]) for rounds in self.L]







def getData(FILE, G, DIR, rounds, TYPE2):
	with open(FILE) as FH:
		for line in FH:
			if "#" == line[0]:
				H,N 	= line.split(",")
				if H not in G:
					G[H] 	=  INFO(H)
			elif "~" == line[0]:
				K,ll,converged, diff 	= line[1:].split(",")
				K,ll,converged, diff 	= float(K), float(ll), float(converged), float(diff)
				G[H].insert_data(rounds, K,ll,converged, diff, DIR, TYPE2)


def RI_directory(directory):
	G 	= {}
	for DIR  in os.listdir(directory):
		TYPE 	= len(DIR.split("_")[-1])
		if TYPE==2:
			TYPE2=int(DIR.split("_")[-1])
		else:
			TYPE2=None
		for FILE in os.listdir(directory+DIR):
			rounds 	= int(FILE.split("_")[-1])
			getData( directory+DIR+"/"+FILE, G, TYPE, rounds, TYPE2)

	return G





		



class MPI_openMP:
	def __init__(self, nodes, threads):
		self.nodes 			= nodes
		self.threads  		= threads
		self.template 		= (None, None)
		self.MLE_root 		= (None, None)
		self.total 			= [0,0]
	def add_time(self, line):
		if "WT: " in line:

			line, WT  	= line.strip("\n").split("WT: ")
			line, CPU 	= line.strip("\n").split("CPU: ")
			WT, CPU 	= float(WT), float(CPU.strip(","))
			if "MLE" in line and "1st" in line:
				self.MPI_root 	= (CPU, WT)
			if "running template matching:" in line:
				self.template 	= (CPU, WT)
			self.total[0]+=CPU
			self.total[1]+=WT

def get_MPI_openMP_clocking_info(FILE):
	with open(FILE) as FH:
		threads,nodes,M 	= None, None,None
		for line in FH:
			if M is not None:
				M.add_time(line)
			else:
				line_array 	= re.split(":", line.strip("\n"))
				if len(line_array) ==2:
					if line_array[0][:3]=="-np" or line_array[0][:9]=="-threads":
						threads 	= int(line_array[1])
					elif line_array[0][:10]=="-MPI_nodes":
						nodes 		= int(line_array[1])
				if threads!= None and nodes!=None:
					M 	= MPI_openMP(nodes, threads)
	return M



def iterate_through_and_organize(directory):
	G 	= {}
	for FILE  in os.listdir(directory):
		M 	= get_MPI_openMP_clocking_info(directory+"/"+FILE)
		if M.nodes not in G:
			G[M.nodes] 	= list()
		G[M.nodes].append((M.nodes, M))
	for t in G:
		G[t].sort()
	return G
		
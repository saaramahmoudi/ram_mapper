mapper_Stratix_IV: mapper_Stratix_IV.cpp 
	g++ -O3 -std=c++11 mapper_Stratix_IV.cpp -o mapper_Stratix_IV

mapper_Stratix_IV_Run:
	./mapper_Stratix_IV 
	./checker -d logical_rams.txt logic_block_count.txt Stratix-IV-like.txt


mapper_general: mapper_general.cpp 
	g++ -O3 -std=c++11 mapper_general.cpp -o mapper_general

mapper_general_Run:
	python3 best_architecture.py > different_architecture_res.txt



	 
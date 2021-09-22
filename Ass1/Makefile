.PHONY: hello seq_sieve clean

hello:
	@mkdir build && cd build && cmake .. && cmake --build .

seq_sieve:
	@g++-11 seq_sieve.cpp -o seq_sieve && ./seq_sieve

clean:
	@rm -rf build *.o seq_sieve
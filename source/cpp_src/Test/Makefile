benchmark: benchmark.cu ../CUDASpectrometer/src/spectrometer.cu  ../CUDASpectrometer/src/noise_statistics_mbp_reduce.cu
	nvcc -DHOSE_USE_ADQ7 -I../CUDASpectrometer/include -o benchmark ../CUDASpectrometer/src/spectrometer.cu benchmark.cu ../CUDASpectrometer/src/noise_statistics_mbp_reduce.cu -lcufft

test: benchmark validate.py
	./benchmark
	python3 validate.py

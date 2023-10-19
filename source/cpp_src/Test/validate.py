import numpy as n
import matplotlib.pyplot as plt
import scipy.signal as ss
import time

s=n.array(n.fromfile("noise.bin",dtype=n.int16),dtype=n.float32)/65535.0
nspec=64
fftlen=2097152
wf=ss.blackmanharris(fftlen)

tmp=n.fft.rfft(n.zeros(fftlen))
S=n.zeros(len(tmp))
t0=time.time()
for i in range(nspec):
    S+=n.abs(n.fft.rfft(s[(i*fftlen):(i*fftlen+fftlen)]))**2.0
t1=time.time()
samps_per_sec=(nspec*fftlen)/(t1-t0)
print("sr %1.2g"%(samps_per_sec))

gpus=n.fromfile("spec.bin",dtype=n.float32)

plt.plot((S[0:int(fftlen/2)]-gpus[0:int(fftlen/2)])/S[0:int(fftlen/2)])
plt.xlabel("Frequency bin")
plt.ylabel("Relative difference $(P_0(\omega)-P_1(\omega))/P_0(\omega)$")
plt.show()

plt.plot(10.0*n.log10(S[0:int(fftlen/2)]))
plt.plot(10.0*n.log10(gpus[0:int(fftlen/2)]))
plt.show()

# frequ in MHz
f=18.432
# Master clock:
#ftarg=64.0
# USB Clock:
ftarg=48.0

# PLL range: 60 MHz <= PLL <= 130 MHz
# MUL range: 4 <= MUL <= 7

min_err_val=[1.0, 0.0]
min_err=f

#for mul in range(1, 8):
for mul in range(1, 36):
    #for div in range(1, 0x20):
    for div in range(1, 255):
        freq=(f*mul/(1.0*div))
        err=abs(freq - ftarg)
        print(mul, div, freq, err)
        if(err < min_err):
            min_err_val=[mul, div]
            print("last minerr: %f, new minerr: %f, mul: %d, div: %d" % (min_err, err, mul, div))
            min_err=err

print("MUL-DIV values:")
print(min_err_val)
print("Absolute error: %0.2f %% " % ((f*min_err_val[0]/min_err_val[1]-ftarg)/ftarg*100))

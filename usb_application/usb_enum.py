import usb.core
import usb.util
import sys

dev = usb.core.find(idVendor=0x03eb, idProduct=0x6004)

if dev is None:
    raise ValueError("Device not found")
else:
    print("Found device")

dev.set_configuration()

cfg = dev.get_active_configuration()
print("Active config: ")
print(cfg)
print("NumConfigs: " + str(dev.bNumConfigurations))
#print(cfg.configurations)


print("***")
for cfg in dev:
   print("*** Next configs: ")
   print(cfg)

print("~~~~~~")
#cfg = usb.util.find_descriptor(dev, bConfiguration=0)

# nur config(1) funktioniert
# config(0): Device haengt
# config(2): usb.core.USBError: [Errno 2] Entity not found
print("dev.set_configuration(" + sys.argv[1] + ")")
dev.set_configuration(int(sys.argv[1]))

print("*** New config: ")
cfg = dev.get_active_configuration()
print(cfg)



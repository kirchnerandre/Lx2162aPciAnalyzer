lx2162a_device_ss = ss.source_set()
lx2162a_device_ss.add(files(
    'lx2162a_device.c',
))

system_ss.add_all(when: 'CONFIG_LX2162A_DEVICE', if_true: lx2162a_device_ss)

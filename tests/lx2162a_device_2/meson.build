lx2162a_device_2_ss = ss.source_set()
lx2162a_device_2_ss.add(files(
    'lx2162a_device_2.c',
))

system_ss.add_all(when: 'CONFIG_LX2162A_DEVICE_2', if_true: lx2162a_device_2_ss)

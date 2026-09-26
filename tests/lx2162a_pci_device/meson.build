pci_d3_good_ss = ss.source_set()
pci_d3_good_ss.add(files(
    'lx2162a_pci_device.c',
))

system_ss.add_all(when: 'CONFIG_LX2162A_PCI_DEVICE', if_true: pci_d3_good_ss)

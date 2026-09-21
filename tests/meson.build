pci_d3_good_ss = ss.source_set()
pci_d3_good_ss.add(files(
    'pci_d3_good.c',
))

system_ss.add_all(when: 'CONFIG_PCID3GOOD', if_true: pci_d3_good_ss)

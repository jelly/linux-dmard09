DMARD09 Linux driver
====================

Mainline Linux dmard09 driver

DTS
---

To add this driver to your arm device tree file.

```
&i2c1 {
	    dmard09@0x1d {
		compatible = "domintech,dmard09";
		reg = <0x1d>;
	    };
};
```

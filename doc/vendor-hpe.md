# Hewlett Packard Enterprise

## OSFCI

The OSFCI @HPE is a service to test firmware builds on real hardware. Originated
at HPE its sources are now maintained under the umbrella of the Open Compute
Project on [GitHub](https://github.com/opencomputeproject/OSF-OSFCI).

### Requirements

- an HPE account, requestable via the Webservice
- a correctly signed image to test (see [Signing Key](board-hpe_proliant_gen11.md#signing-key))

### Webservice

There are two known instances which only differ in the location and the machines
they provide:

- [osfci.tech/ci](https://osfci.tech/ci/)
- [eu.osfci.tech/ci](https://eu.osfci.tech/ci/)

For either the workflow is the same:

1. Login 
> [!NOTE]
> the websession timeouts after a while when idle but doesn't indicate that
> directly. It's still possible to request new sessions and kind of interact
> with it, but nothing really happens. The only indication is that the main page
> gets shown in iFrames where console output should been visible.
2. Request a machine via the drop-down menu on the top right. Select your user
e-mail, then "Interactive Session" and choose a target type.
3. Drag and drop the firmware to the indicated section on top. The upload process
is indicated and the console output of the flasher gets printed on the right.
4. When the upload process is finished start the machine with the "connect power"
button at the bottom section. The console output of the starting BMC is again
printed on the right.
5. After booting up it is possible to login to the console on the right.
(standard test credential: `root`/`0penBmc`)
6. It's also possible to reach the Webinterface via the "BMC Web" button on the
left. The button will appear once a running web instance is discovered.
7. When finished it's crucial to end the session via the red button at the top
which ensures the proper release of the machine so it's ready again to serve
another session.


### CLI

The canopy project also developed `osfci-cli`, a tool to use the OSFCI service
via the command line. Instructions are provided at the [repository](https://github.com/canopybmc/osfci-cli).


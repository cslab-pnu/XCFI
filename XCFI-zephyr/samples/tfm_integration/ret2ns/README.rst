.. zephyr:code-sample:: tfm_ret2ns
   :name: TF-M ret2ns evaluation

   Exercise ret2ns-style control-flow attacks against direct TF-M NSC veneers.

Overview
********

This sample is a non-secure Zephyr application that calls direct secure gateway
functions added to TF-M's NS Agent. It is intended for security evaluation, not
as a normal PSA IPC service example.

The sample provides four variants selected by ``CONFIG_RET2NS_ATTACK_ID``:

* ``1``: NS privileged thread calls the BXNS-style vulnerable secure veneer (backward-edge).
* ``2``: NS privileged thread calls the BLXNS-style vulnerable secure veneer (forward-edge).

Build
*****

.. code-block:: console

   ./build_with_tfm.sh samples/tfm_integration/ret2ns base

For XCFI/PACBTI modes, pass the existing build script mode argument, for example:

.. code-block:: console

   ./build_with_tfm.sh samples/tfm_integration/ret2ns xcfi


# AMX Crosspoint DLL

This DLL allows to control a crosspoint from [AMX](https://www.amx.com) in your [ProfiLab](http://www.abacom-online.de/html/profilab.html) project.

![Screenshot](module.png)

## Input Pins

Name   | Value Range       | Value Interpretation
-------|-------------------|---------------------
STORE  | 0 .. ?            | 0: do nothing<br>*n*: Store to preset *n*
RECALL | 0 .. ?            | 0: do nothing<br>*n*: Recall from preset *n*
OUT*n* | 0 .. *num inputs* | 0: Clear the output *n*<br>*x*: Route input *x* to output *n*

For all pins decimal places are cut off. **DO NOT** pass negative numbers, the behavior is undefined.


## Output Pins

Name   | Value Range       | Value Interpretation
-------|-------------------|---------------------
CONN   | 0 .. 5            | 0: not connected<br>1: connected
ERR    | 0 .. 5            | 0: no error<br>1: error
$ERR   |                   | Textual representation of the error
OUT*n* | 0 .. *num inputs* | 0: No input is routed to output *n* <br>*x*: Input *x* is routed to output *n*

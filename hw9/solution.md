# Homework 9

The section on conditional variables [here](https://computing.llnl.gov/tutorials/pthreads/#ConditionVariables) was very helpful.

The homework wasn't too difficult after reading the above tutorial, but I'd like to emphasize one thing. The `if` statement is only entered by the last thread to join the barrier and it is that thread's responsibility to set the `bstate.nthreads` field back to 0. That ensures that that same thread cannot loop around and re-release the barrier before all other threads have finished executing. If that happened then the threads that didn't get a chance to be released will be behind by one round.

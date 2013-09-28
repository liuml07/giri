This is for indirect recursive function calls.

A standard example of (indirect) mutual recursion, which is admittedly artificial, is determining whether a non-negative number is even or is odd by having two separate functions and calling each other, decrementing each time. These functions are based on the observation that the question is 4 even? is equivalent to is 3 odd?, which is in turn equivalent to is 2 even?, and so on down to 0.

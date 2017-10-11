# Miniature Shell (msh)
This is a miniature shell written in C that comes with some basic features.
- Background processes: `/path_to/some_long_running_process &`
- Foreground process stopping/killing with `^Z`, `^C`
- bg/fg process commands to bring a stopped or background process to foreground/background
- A list of background/stopped jobs with `jobs`

## Files:
<pre>
Makefile	# Compiles the shell program and runs the tests
psh.c           # Puny shell with a simple fg task runner
msh.c		# Miniature shell with fg/bg jobs and basic signal handling
mshref		# The reference shell binary.
util.c/h        # Contains provided utilities
jobs.c/h        # Contains job helper routines


# Files used to test the shell
sdriver.pl	# The trace-driven shell driver
trace*.txt	# The 16 trace files that control the shell driver
mshref.out 	# Example output of the reference shell on all 16 traces

# Little C programs that are called by the trace files
myspin.c	# Takes argument &lt;n&gt; and spins for &lt;n&gt; seconds
mysplit.c	# Forks a child that spins for &lt;n&gt; seconds
mystop.c        # Spins for &lt;n&gt; seconds and sends SIGTSTP to itself
myint.c         # Spins for &lt;n&gt; seconds and sends SIGINT to itself

# Further files to test the shell by hand, or to write tests with
fib.c           # Fibonacci program that spawns child processes to calculate fib(n)
handle.c        # Simple signal handling program that prevents ^C, ^Z from killing/stopping it
mykill.c        # Simple program that sends SIGUSR1 to a PID
</pre>

## Credits
This shell program was adapted from the University of Texas of Austin's shell lab for CS439, which I believe was originally from the Computer Systems - A Programmer's Perspective (Bryant, O'Hallaron) book resources. A license was not attached to the files given to me, but I intend to respect any licenses originally endowed on the original project files.

/*
Let's pretend we want to time some code with our poorman's profiler.  The C version is rather straightforward:

```
    #include <sys/time.h> // gettimeofday()             // 1

    timeval start, end;                                 // 2

    gettimeofday( &start, NULL );                       // 3
        benchmark_c();                                  // 4
    gettimeofday( &end, NULL );                         // 5
    double ms = (end.tv_sec - start.tv_sec) * 1000.f;   // 6
```

For `benchmark_c()` I'm using a dummy wait:

```
    #include <unistd.h>   // sleep()

    void benchmark_c()
    {
        sleep(1);
    }
```

However, compiling on MSVC / Windows we need to address the facts that:

 * MSVC doesn't include the standard `gettimeofday()` !?!?
 * MSVC doesn't have `<sys/time.h>` ?
 * MSVC has Sleep() in `<Windows.h>`; not the standard sleep() in `<unistd.h>`

**sigh**

Here is a portable wrapper for gettimeofday() and sleep():

```
*/

#ifdef _WIN32 // MSVC
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h> // Windows.h -> WinDef.h defines min() max()
    #include <stdint.h>  // uint64_t

    // *sigh* Microsoft has this in winsock2.h !?!?
    typedef struct timeval {
        long tv_sec;
        long tv_usec;
    } timeval;

   // *sigh* no gettimeofday on Win32/Win64
    int gettimeofday(struct timeval * tp, struct timezone * tzp)
    {
        // FILETIME Jan 1 1970 00:00:00
        // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
        static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL); 

        SYSTEMTIME  nSystemTime;
        FILETIME    nFileTime;
        uint64_t    nTime;

        GetSystemTime( &nSystemTime );
        SystemTimeToFileTime( &nSystemTime, &nFileTime );
        nTime =  ((uint64_t)nFileTime.dwLowDateTime )      ;
        nTime += ((uint64_t)nFileTime.dwHighDateTime) << 32;

        tp->tv_sec  = (long) ((nTime - EPOCH) / 10000000L);
        tp->tv_usec = (long) (nSystemTime.wMilliseconds * 1000);
        return 0;
    }

    unsigned int sleep(unsigned int seconds)
    {
        Sleep(seconds * 1000); // takes milliseconds
        return 0;
    }
#else
    #include <sys/time.h> // gettimeofday
    #include <unistd.h>   // sleep()
#endif


// ____________________________________________________________________________________________________


/*
```

As I listed above notice how tiny the C version is -- a mere 6 Lines of Code!

**NOTE:** I commented out the standard header includes since they are included above with our wrapper.

```
*/

    #include <stdio.h>
//  #include <sys/time.h> // gettimeofday()                 // 1
//  #include <unistd.h>   // sleep()

    // ====================================================================
    void benchmark_c()
    {
        sleep(1);
    }

    // ====================================================================
    void demo_c_time()
    {
        timeval start, end;                                 // 2

        gettimeofday( &start, NULL );                       // 3
            benchmark_c();                                  // 4
        gettimeofday( &end, NULL );                         // 5
        double ms = (end.tv_sec - start.tv_sec) * 1000.f;   // 6

        printf( "C   milli: %f\n", ms );
    }

/*
```

"Naturally" the native C++ "solution" is to use `std::chrono` and one of the clocks such as `std::chrono::high_resolution_clock`.

Compare and contrast the minimal plain C version above to this Crap++ solution:

```
*/

    #include <stdio.h>
    #include <chrono>                                                                                                   // 1
    #include <thread> // std::this_thread::sleep_for()

    // ====================================================================
    void benchmark()
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // ====================================================================
    void demo_ms_verbose_ugly()
    {
        std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();               // 2
            benchmark();                                                                                                // 3
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();                 // 4

        std::chrono::duration<double, std::milli> delta_ms = end - begin;                                               // 5
        std::chrono::milliseconds milliseconds = std::chrono::duration_cast< std::chrono::milliseconds >( delta_ms );   // 6
        double ms = (double) milliseconds.count();                                                                      // 7

        printf( "C++ milli: %f\n", ms );
    }

/*
```

UGH.

W.T.F.

Why do I need 120 chars just to read this???

Even though there are roughly the same number of lines all this extra verbose CRAP is hard to read.
That is, the S:N (Signal-to-Noise) ratio of C++ is far WORSE due to all these namespaces.

Let's add some multi-column ALIGNMENT to improve readability:

```
*/

    // ====================================================================
    void demo_ms_verbose_bad()
    {
        std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
            benchmark();
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> delta_ms     = end - begin;
        std::chrono::milliseconds                 milliseconds = std::chrono::duration_cast< std::chrono::milliseconds >( delta_ms );
        double                                    ms           = (double) milliseconds.count();

        printf( "B++ milli: %f\n", ms );
    }

/*
```

OK, that's a _little_ better but notice how with this wide code we are basically forced to have even MORE width just to see the code!
Before we continue we need to briefly return to the first C++ example.
    
Due to C++'s bloated namespaces and shitty templates figuring out the ACTUAL type is a clusterfuck of wasting your time.
The first example is where people usually succumb to the EXCUSE of using 'auto' just to minimize this "noise".

**HINT:** If you basically HAVE to use an IDE just to figure this template shit out -- then your language/frameworks SUCKS!

```
*/

    // ====================================================================
    void demo_ms_verbose_auto_crap()
    {
        std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
            benchmark();
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

        auto delta_ms = end - begin;
        auto milliseconds = std::chrono::duration_cast< std::chrono::milliseconds >( delta_ms );
        double ms = (double) milliseconds.count();

        printf( "D++ milli: %f\n", ms );
    }

/*
```

With auto we gained a _little_ readability at the _HUGE_ expense of KNOWING what the actual fucking types are.

Again, let's clean this crap up.

*/
    // ====================================================================
    void demo_ms_verbose_auto_better()
    {
        std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
            benchmark();
        std::chrono::high_resolution_clock::time_point end   = std::chrono::high_resolution_clock::now();

        auto   delta_ms     = end - begin;
        auto   milliseconds = std::chrono::duration_cast< std::chrono::milliseconds >( delta_ms );
        double ms           = (double) milliseconds.count();

        printf( "B-- milli: %f\n", ms );
    }

/*
```

This code is SLOWLY getting better but still cluttered.  Namely, verbose names and that stupid duration cast.

So how do we proceed cleaning up this junk?

First, let's setup some typedefs to manage the complexity.  Notice that

```
    std::chrono::milliseconds
```

is NOT the same as:

```
    std::milli
```

The former is defined as:

```
    typedef duration<long long, milli> milliseconds;
```

While the latter is defined as:

```
    typedef ratio<1, 1000> milli;
```

This is one of the reasons Crap++ is slow to compile -- layers and layers of shit masqeurading the underlying type(s).
At the end of a day just give me a fucking "double" instead of this excessive type checking that basically
requires a cast to access.

```
*/
    typedef std::chrono::high_resolution_clock             hrclock_t;
    typedef std::chrono::high_resolution_clock::time_point hrtime_t;

    typedef std::chrono::milliseconds time_milli_t;
    typedef std::chrono::microseconds time_micro_t;
    typedef std::chrono::nanoseconds  time_nano_t;

    typedef std::chrono::duration<double, std::milli> dur_ms;
    typedef std::chrono::duration<double, std::micro> dur_us;
    typedef std::chrono::duration<double, std::nano > dur_ns;

    // ====================================================================
    void demo_ms_compact_good()
    {
        hrtime_t begin = hrclock_t::now();
            benchmark();
        hrtime_t end = hrclock_t::now();

        dur_ms delta_ms = end - begin;
        double ms = (double) std::chrono::duration_cast< time_milli_t >( delta_ms ).count();

        printf( "A-- milli: %f\n", ms );
    }

/*
```

That's better but we can actually use a few macros to even get rid of that bloat cast.

Now sometime we want integral time values for the full 64-bit integer precision instead of 53-bit "double" mantissaa precision.
We could do something like this ...

```
    #ifdef TIMER_INT64
        typedef  uint64_t Time_Type;
    #else
        typedef  double Time_Type;
    #endif
```

... and ...

```
    #ifdef TIMER_INT64
        printf( "milli: %llu\n", ms );
    #else
        printf( "milli: %f\n", ms );
    #endif
```

... but we'll default to floating-point to minimize visual clutter.
If we really wanted it would be easy enough to use integer output via:

```
    typedef  uint64_t elapsed_t;
    printf( "milli: %llu\n", ms );
```

A typedef and a few pre-processor macros ...

```
*/

    typedef double elapsed_t;
    #define DURATION_TO_INTERVAL(delta,interval)                        \
        (elapsed_t)std::chrono::duration_cast<interval>(delta).count();

    #define DURATION_TO_MS(delta) DURATION_TO_INTERVAL(delta,time_milli_t)
    #define DURATION_TO_US(delta) DURATION_TO_INTERVAL(delta,time_micro_t)
    #define DURATION_TO_NS(delta) DURATION_TO_INTERVAL(delta,time_nano_t )

    // ====================================================================
    void demo_ms_compact_better()
    {
        hrtime_t begin = hrclock_t::now();
            benchmark();
        hrtime_t end = hrclock_t::now();

        dur_ms delta_ms = end - begin;
        elapsed_t ms = DURATION_TO_MS( delta_ms );

        printf( "A++ milli: %f\n", ms );
    }

/*
```

... add a dash of alignment and Bob's your uncle.

```
*/
    // ====================================================================
    void demo_ms_compact_best()
    {
        hrtime_t begin = hrclock_t::now();
            benchmark();
        hrtime_t end  = hrclock_t::now();

        dur_ms    delta_ms = end - begin;
        elapsed_t ms       = DURATION_TO_MS( delta_ms );

        printf( "S   milli: %f\n", ms );
    }

/*
```

We added an total of 14 (8 + 6) extra lines of C++ code to make the timing code less harder to read.
While we only have to do this once this is systemic of C++'s "noise."

At least the output matches, right? If only we could get an OS with better then ms accuracy.  /s

    C   milli: 1000.000000
    C++ milli: 1000.000000
    B++ milli: 1001.000000
    D++ milli: 1000.000000
    B-- milli: 1000.000000
    A-- milli: 1000.000000
    A++ milli: 1000.000000
    S   milli: 1000.000000
    Done.

I'm going to ignore the clusterfuck of portable chrono precision:

```
    // On some systems supposedly outputs 1 ns.  Yeah, right, we have 1 ns precision -- no on Windows!  **FACEPALM**
    std::cout << (double)std::chrono::high_resolution_clock::period::num / std::chrono::high_resolution_clock::period::den;
```

 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63400
 * https://stackoverflow.com/questions/48211688/how-accurate-is-stdchrono


C++'s chrono is also extremely painfull to debug if you are used to viewing the disassembly. This single line of C++ code:

```
    auto now = std::chrono::high_resolution_clock::now();
```

Generates this crapfest disassembly:

```
    call        std::chrono::time_point<std::chrono::system_clock,std::chrono::duration<__int64,std::ratio<1,10000000> > >::time_point<std::chrono::system_clock,std::chrono::duration<__int64,std::ratio<1,10000000> > >
```

For contrast here is the call to our wrapper:

```
    call        gettimeofday
```

And you wonder why old-skool programmers complain about how much **visual vomit** C++ introduces?
Is there any doubt left why I refer to it as Crap++? /s

*/


// ____________________________________________________________________________________________________


// ====================================================================
void dump_elapsed( const elapsed_t ms, const elapsed_t us, const elapsed_t ns )
{
//    std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
//        benchmark();
//    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
//    std::chrono::duration<double, std::milli> delta_ms = end - begin;

    #ifdef TIMER_INT64
        printf( "milli: %llu\n", ms );
        printf( "micro: %llu\n", us );
        printf( "nano : %llu\n", ns );
    #else
        printf( "milli: %f\n", ms );
        printf( "micro: %f\n", us );
        printf( "nano : %f\n", ns );
    #endif
}


// ____________________________________________________________________________________________________


    // ====================================================================
    void demo_time_units_compact()
    {
        hrtime_t begin = hrclock_t::now();
            benchmark();
        hrtime_t end = hrclock_t::now();
        dur_ms delta_ms = end - begin;

        elapsed_t ms = DURATION_TO_MS( delta_ms );
        elapsed_t us = DURATION_TO_US( delta_ms );
        elapsed_t ns = DURATION_TO_NS( delta_ms );
        dump_elapsed( ms, us, ns );
    }

    // ====================================================================
    void demo_time_units_compare()
    {
        printf( "\n=== Converting ms to ns precision ===\n" );

        // std::chrono::nanoseconds
        hrtime_t begin = hrclock_t::now();
            benchmark();
        hrtime_t end = hrclock_t::now();

        dur_ms delta_ms = end - begin;
        dur_us delta_us = end - begin;
        dur_ns delta_ns = end - begin;

        elapsed_t ms = DURATION_TO_MS( delta_ms );
        elapsed_t us = DURATION_TO_US( delta_ms );
        elapsed_t ns = DURATION_TO_NS( delta_ms );

        dump_elapsed( ms, us, ns );
        printf( "\n" );

            us = DURATION_TO_US( delta_us );
            ns = DURATION_TO_NS( delta_ns );

        dump_elapsed( ms, us, ns );
        printf( "\n" );
    }


// ____________________________________________________________________________________________________

    void dump_generic_clock_metrics( const char *name, const double numerator, const double denominator, const double actual_quanta_ns )
    {
        const double expected_quanta_s  = (numerator / denominator);
        const double expected_quanta_ms = expected_quanta_s  * 1000.;
        const double expected_quanta_us = expected_quanta_ms * 1000.;
        const double expected_quanta_ns = expected_quanta_us * 1000.;
        printf( "    %s: %.3f / %9.1f = %9.8f seconds = %5.2f nanoseconds", name, numerator, denominator, expected_quanta_s, expected_quanta_ns );
        printf( " (minimum quanta: %f ns)\n", actual_quanta_ns );
    }


    // ====================================================================
    void dump_timer_specs()
    {
        typedef std::chrono::steady_clock stdclock_t;
        typedef std::chrono::system_clock sysclock_t;

        typedef stdclock_t::time_point stdtime_t;
        typedef sysclock_t::time_point systime_t;
        
        printf( "=== Minimal Timer Quanta ===\n" );

        // DRY (Don't Repeat Yourself) Cleanup:
        // printf( "    high resolution clock : %f / %f = %9.8f seconds = %f nanoseconds\n", hrc_n, hrc_d, hrc_s, hrc_ns );
        // printf( "    steady clock precision: %f / %f = %9.8f seconds = %f nanoseconds\n", std_n, std_d, std_s, std_ns );
        // printf( "    system clock precision: %f / %f = %9.8f seconds = %f nanoseconds\n", sys_n, sys_d, sys_s, sys_ns );

        const double   hrc_n      = (double) hrclock_t::period::num;
        const double   hrc_d      = (double) hrclock_t::period::den;
              hrtime_t hrc_start  = hrclock_t::now();
              hrtime_t hrc_end    = hrclock_t::now();
              double   hrc_quanta = DURATION_TO_NS( (hrc_end - hrc_start) );
        dump_generic_clock_metrics( "high resolution clock ", hrc_n, hrc_d, hrc_quanta );

        const double    std_n      = stdclock_t::period::num;
        const double    std_d      = stdclock_t::period::den;
              stdtime_t std_start  = stdclock_t::now();
              stdtime_t std_end    = stdclock_t::now();
              double    std_quanta = DURATION_TO_NS( (std_end - std_start) );
        dump_generic_clock_metrics( "steady clock precision", std_n, std_d, std_quanta );

        const double    sys_n      = sysclock_t::period::num;
        const double    sys_d      = sysclock_t::period::den;
              systime_t sys_start  = sysclock_t::now();
              systime_t sys_end    = sysclock_t::now();
              double    sys_quanta = DURATION_TO_NS( (sys_end - sys_start) );
        dump_generic_clock_metrics( "system clock precision", sys_n, sys_d, sys_quanta );
        printf( "\n" );
    }


    // ====================================================================
    int main()
    {
        dump_timer_specs();

        demo_c_time();
        demo_ms_verbose_ugly();
        demo_ms_verbose_bad();

        demo_ms_verbose_auto_crap();
        demo_ms_verbose_auto_better();

        demo_ms_compact_good();
        demo_ms_compact_better();
        demo_ms_compact_best();

        demo_time_units_compare();

        printf( "Done.\n" );
        getchar();
}
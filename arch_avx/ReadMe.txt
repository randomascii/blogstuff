This project demonstrates the ODR (One Definition Rule) problems that occur
through using /arch:AVX with anything less than obsessive care.

Just run build.bat and carefully pay attention to the assembly language
implementation of floorf. For details see the associated blog post.

Note that the code doesn't actually detect whether AVX is supported. In fact
its AVXDetect method simply returns true if argc is greater than one. But
this hardly matters since most versions of the program call the same version
of floorf whether AVX is detected or not, so what's the point?

Blog post can be found here:
https://randomascii.wordpress.com/2016/12/05/vc-archavx-option-unsafe-at-any-speed/
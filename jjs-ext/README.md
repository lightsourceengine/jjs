**DEPRECATION WARNING**

The jjs-ext library is deprecated and will be removed in a future release of JJS.

The JJS project is focused on ECMA conformance, JS functionality through packs, performance
and embedability. The jjs-ext lib does not support these goals and I would rather not 
spend cycles keeping this library up to date. The lib seems to primarily be support for the commandline tools and
minimal JJS util methods. If any of this is truly valuable, it could be spun out into a
separate library. The API supporting the commandline tools should be moved to a private
lib for the command line tools to use.

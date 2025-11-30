# Deep dive to JOPA

I wanted to better understand capabilites and applicability ceilings of the latest generation of models (Opus 4.5, Sonnet 4.5, gpt-5.1-codex-max, Gemini 2.5 Pro) 
and learn the ways to use them efficiently. When I tasked Claude to refresh Jikes, I dind't expect it to succeed at all. It outperformed my expectations.

It managed to add Java 7 syntax support and preliminary Java 5 language features in a day in mostly unsupervised mode but it took us a week of hard work to get something which seem to work as a Java 7 compiler. But it's still full of bugs.

Good things I've learned:

- Today (in 2025) models can do much more than 2 years ago. In 2023 I hoped that I could get a single compilation unit done with an agent, now I can think about compilers.
- Models can help us revitalize ancient codebases which humans cannot handle
- Models can work with C++ and produce code which doesn't fail on any sanitizers, including memory leak sanitizers.
- The best language for the models is Python, they deliver much faster when they work with Python codebases. They are especially good in handling modern Python with type annotations.
- Models can debug, read logs and use all the utilities in your toolchain with extremely high efficiency. They are much more knowledgeable than any engineer out there and sometimes they can happily debug Java bytecode by hexdumps.
- You can work with codebases consisting of tens of thousands of lines. Even C++ lines.

Bad things:

- Models cannot abstract well and cannot generalize well. They are like humans and tend to deliver solutions for specific problems they see, but they generalize much less.
- Model outputs may look correct individually but not compose at all. Again, they cannot generalize.
- When unsupervised, they fail spectacularly in large refactorings and cannot design at all (again, incapable of generalization). I've tried to modularize this compiler, decouple components, replace the parser, I've tried to do many other transformations, all that failed, Claude is incapable of deep thinking and planning.
- Models cannot produce correct C++ code which would not have UBs and memory management issues on ALL code paths.
- They tend to take any shortcuts possible and behave like a bad genie.
- Codex and Gemini are MUCH less capable, on projects of this scale (~50000 C++ lines) they cannot produce coherent output at all. Claude is MUCH better. But again, on codebases of this size you cannot perform global tasks with Claude.
- Claude can easily get sidetracked and forget main goal
- Claude's CLI tool has insane memory leaks, the experience is very painful
- Frequently, Claude cannot see "obvious" solutions
- Claude loves to tell you that something alike to "we did a lot, let's make a coffee break". It's hard to make it work in a loop until it delivers.
- Codex and Geminin cannot work in a loop at all. Despite all the effort, they stop fast.
- You have to be always in the loop (more on that below). You cannot leave them unsupervised - they won't deliver.
- Models cannot concentrate on guidelines long enough.
- The models may mess up, delete files, overwrite files and do whatever random shit you can imagine. Don't trust them, isolate them. Commit often, be ready to reset environments.
- Claude cannot implement hard things. Even if it sees the logic of StackMap frame generation in OpenJDK - it cannot generalize it and reproduce here, it did amazing job but the implementation is still failing on many test cases.

Usage patterns:

- Be in the loop, monitor what the agent does, think and steer it towards the goal.
- Record important patters into your [CLAUDE.md](./CLAUDE.md). Try to be precise, 
- It might help to setup one model (Codex in my case) as a reviewer, so it would steer another (Claude) towards the goal. Unfortunately, there is no nice and convenient tool to organize such setup (there are some though), so at this point in time it's a good idea to be creative and write (vibe-code) your own.
- Always ensure that your model is working in reproducible environment (like Nix), instruct it to use your environment and tooling, start by writing tests and implementing reproducible builds. Toolings and environments are extremely important for model efficiency. Make your tests and builds fast. Apply TDD. Insist that the model should always run tests and ensure they all pass. It helped a lot to switch build to `CMake` early, add `cpptrace` into the compiler, switch to `CLang` and turn on sanitizers.
- Run models in [FireJail](https://github.com/netblue30/firejail) without their own sandboxes, that's the only way to be at least a bit productive.

And also:

- Java is complicated, there are a lot of things which are simple as concepts but require meticulous attention to be implemented properly (autoboxing, generics/type inference, reflection metadata, stack maps, synthetic methods, bridges, varargs, ...). The mere fact that Claude was able to implement such high level of compliance is impressive.
- It's hard to test Java compilers - you have comprehensive testkits but they are badly organized and hard to use. The worst thing is that there is no separate validation tool - you cannot properly validate your bytecode w/o running it under JVM. You cannot validate that your bytecode is compliant with particular bytecode version without  You need a JVM of a particular version to validate, and these JVMs might be hard to source, install, etc.
- In terms of compiler development and the development culture - we live in a much better world than before. We don't need to optimize compilers for memory and performance, we can use nice parser and multiple versions of immutable syntax trees, one per phase, we can run complex recursive alghoritms and we do write tests - previous generations used to live in a much less pleasant environment, the original compiler is a mess if we look at it from 2025.
- It's funny and sad, that GNU Classpath, Jikes and other ancient tools like JamVM are the only way to bootstrap OpenJDK in our days. If these small project didn't exist, the bootstrap won't be possible at all.
- It's sad that GNU and hacker culture is dying. Most of the stuff we do isn't fun at all.

## Updates

I've tested Gemini 3 Pro and it obliterates Claude in terms of generalization abilities. It has funny flaws, like Claude can edit 10K LoC large C++ files, Gemini 3 Pro repeatedly fails on much smaller files, but at least it can work around by writing Python scripts which patch the files.

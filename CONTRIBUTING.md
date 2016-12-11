
Developer Notes for Contributing Code
=====================================

We welcome and appreciate any kind of use case evaluations, feature requests,
bug reports and code contributions. To simplify cooperation, we follow the
guidelines described in the following.


Developer Notes
---------------

Consult the [Contributor Primer](http://doc.dash-project.org/ContributorPrimer)
in the [DASH Documentation Wiki](http://doc.dash-project.org/) for useful hints
on the code base, build configurations, debugging, HPC system setups etc.


Code Style
----------

We follow the
[Google C++ Style Guide](http://google.github.io/styleguide/cppguide.html)
which is widely accepted in established open source projects.

The
[standards defined by the LLVM team](http://llvm.org/docs/CodingStandards.html)
are worth mentioning, too.



Contributing Code
-----------------

The DASH software development process is kept as simple as possible,
balancing pragmatism and QA.

**1. Create a new, dedicated branch for any task.**

We follow the naming convention:

- `feat-<issue-id>-<shortname>` for implementing and testing features
- `bug-<issue-id>-<shortname>`  for bugfix branches
- `sup-<issue-id>-<shortname>`  for support tasks (e.g. build system,
  														  documentation)
- `doc-<issue-id>-<shortname>`  for doxygen and userguide documentation. Does not run through CI.

For example, when fixing a bug in team allocation, you could name your branch
*bug-team-alloc*.

**2. Submit an issue on GitHub**

- State the name of your branch in the ticket description.
- Choose appropriate tags (support, bug, feature, ...).
- As a member in the DASH project, assign the ticket to the owner of the
  affected module.


**3. For features and bugfixes, implement unit tests**

The integration test script can be used to run tests in Debug and
Release builds with varying number of processes automatically:

    (dash-root)$ ./dash/scripts/dash-ci.sh

Check if CI succeeded on
[Travis](https://travis-ci.org/dash-project/dash)
and
[CircleCI](https://circleci.com/gh/dash-project/dash).


**4. Once you are finished with your implementation and unit tests:**

- Create a new pull request on GitHub.
- Assign the pull request to one or more members in the DASH team
  for review.


**5. Closing a ticket**

Issues are closed by the assigned reviewers once the related pull request
passed review and has been merged.


**6. Merging to master**

The development branch of DASH is merged to master periodically once all
issues in the development branch associated with the next stable release
are closed.

Before merging:

- Update CHANGELOG in development branch.
- Announce the merge: send the updated changelog to the DASH mailing
  list and set a deadline for review, testing, and pending issue reports.

After merging:

- Tag the merge commit with the version number of the release, e.g.
  `dash-0.5.0`
- Increment the DASH version number in the development branch, e.g from
  0.5.0 to 0.6.0.
- Publish a new release: create a tarball distribution by running
 `release.pl` and add a link on the DASH project website.


#How to commit changes

# Prerequisites #

Some SVN client, e.g. SlikSVN for Windows

# Commands #

1. Checkout the source

svn checkout svn checkout https://shot-glass-tracking.googlecode.com/svn/trunk/ shot-glass-tracking --username QuickAndDirtyFOV

2. Modify files, add files etc, then:

svn add <new files, modified files>

3. Commit

svn commit -m "First Commit" --username QuickAndDirtyFOV --password 

&lt;pwd&gt;



Use the generated pwd
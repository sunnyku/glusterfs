#!/bin/bash

. $(dirname $0)/../include.rc
. $(dirname $0)/../nfs.rc

cleanup;


## Start and create a volume
TEST glusterd
TEST pidof glusterd
TEST $CLI volume info;

TEST $CLI volume create $V0 replica 2 $H0:$B0/${V0}{1,2,3,4,5,6,7,8};

## Start volume and verify
TEST $CLI volume start $V0;

## Mount FUSE with caching disabled (read-write)
TEST $GFS -s $H0 --volfile-id $V0 $M0;

TEST ! stat $M0/subdir1;
TEST mkdir $M0/subdir1;
TEST ! stat $M0/subdir2;
TEST mkdir $M0/subdir2;
TEST ! stat $M0/subdir1/subdir1.1;
TEST mkdir $M0/subdir1/subdir1.1;
TEST ! stat $M0/subdir1/subdir1.1/subdir1.2;
TEST mkdir $M0/subdir1/subdir1.1/subdir1.2;

# mount volume/subdir1
TEST $GFS --subdir-mount subdir1 -s $H0 --volfile-id $V0 $M1;

TEST touch $M0/topfile;
TEST ! stat $M1/topfile;

TEST touch $M1/subdir1_file;
TEST ! stat $M0/subdir1_file;
TEST stat $M0/subdir1/subdir1_file;

# mount volume/subdir2
TEST $GFS --subdir-mount subdir2 -s $H0 --volfile-id $V0 $M2;

TEST ! stat $M2/topfile;

TEST touch $M2/subdir2_file;
TEST ! stat $M0/subdir2_file;
TEST ! stat $M1/subdir2_file;
TEST stat $M0/subdir2/subdir2_file;

# umount $M1 / $M2
TEST umount $M1
TEST umount $M2

# mount non-existing subdir ; this works with mount.glusterfs,
# but with glusterfs, the script doesn't returns error.
#TEST ! $GFS --subdir-mount subdir_not_there -s $H0 --volfile-id $V0 $M1;

# mount subdir with depth
TEST $GFS --subdir-mount subdir1/subdir1.1/subdir1.2 -s $H0 --volfile-id $V0 $M2;
TEST ! stat $M2/topfile;
TEST touch $M2/subdir1.2_file;
TEST ! stat $M0/subdir1.2_file;
TEST stat $M0/subdir1/subdir1.1/subdir1.2/subdir1.2_file;


TEST $CLI volume stop $V0;
TEST $CLI volume delete $V0;
TEST ! $CLI volume info $V0;

## This should clean the mountpoints
cleanup;

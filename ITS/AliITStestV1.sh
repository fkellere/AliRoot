#!/bin/sh

# delete eventual old files from the last run
./AliITSDeleteOldFiles.sh

# run the hit generation
aliroot -q -b "$ALICE_ROOT/macros/grun.C"

# digitize TPC
aliroot -q -b "$ALICE_ROOT/TPC/AliTPCHits2Digits.C"

# find clusters in TPC
aliroot -q -b "$ALICE_ROOT/TPC/AliTPCFindClusters.C"

# find tracks in TPC
aliroot -q -b "$ALICE_ROOT/TPC/AliTPCFindTracks.C"

# do TPC tracking comparison
aliroot -q -b "$ALICE_ROOT/TPC/AliTPCComparison.C"

# create summable digits for the ITS
aliroot -q -b "$ALICE_ROOT/ITS/AliITSHits2SDigits.C"

# go from summable digits to digits
aliroot -q -b "$ALICE_ROOT/ITS/AliITSSDigits2Digits.C"

# create reconstructed points for the ITS
aliroot -q -b "$ALICE_ROOT/ITS/AliITSDigits2RecPoints.C"

# do the tracking
aliroot -q -b "$ALICE_ROOT/ITS/AliITSTrackingV1.C"

# prepare for comparison
# aliroot -q -b "$ALICE_ROOT/ITS/AliITSTracksV1.C"
aliroot -q -b "$ALICE_ROOT/ITS/AliITSStoreFindableTracks.C"

# do ITS tracking comparison
aliroot -q -b "$ALICE_ROOT/ITS/AliITSComparisonV1.C"

#
# after all of the above you can run AliITSPlotTracksV1.C macro under 
# aliroot to see plots of the efficiency and track parameter resolution
# or AliITSDisplayTracksV1.C to display found tracks on top of the
# ITS detailed geometry

# shellcheck disable=SC1113
# /*
#  * (C) Copyright 2016-2023 Intel Corporation.
#  *
#  * SPDX-License-Identifier: BSD-2-Clause-Patent
# */

set -eux

# check that vm.max_map_count has been configured/bumped
if [ "$(sudo sysctl -n vm.max_map_count)" -lt "1000000" ] ; then
    echo "vm.max_map_count is not set as expected"
    exit 1
fi

if $TEST_RPMS; then
    rm -rf "$PWD"/install/tmp
    mkdir -p "$PWD"/install/tmp
    # set the shared dir
    # TODO: remove the need for a shared dir by copying needed files to
    #       the test nodes
    export DAOS_TEST_SHARED_DIR=${DAOS_TEST_SHARED_DIR:-$PWD/install/tmp}
    logs_prefix="/var/tmp"
else
    rm -rf "$DAOS_BASE"/install/tmp
    mkdir -p "$DAOS_BASE"/install/tmp
    logs_prefix="$DAOS_BASE/install/lib/daos/TESTING"
    cd "$DAOS_BASE"
fi

# Disable CRT_PHY_ADDR_STR to allow launch.py to set it
unset CRT_PHY_ADDR_STR

# Disable OFI_INTERFACE to allow launch.py to pick the fastest interface
unset OFI_INTERFACE

# At Oct2018 Longmond F2F it was decided that per-server logs are preferred
# But now we need to collect them!  Avoid using 'client_daos.log' due to
# conflicts with the daos_test log renaming.
# shellcheck disable=SC2153
export D_LOG_FILE="$TEST_TAG_DIR/daos.log"

# apply patches to Avocado
pydir=""
for loc in /usr/lib/python2*/site-packages/ \
           /usr/lib/python3*/site-packages/ \
           /usr/local/lib/python3*/site-packages/; do
    if [ -f "$loc"/avocado/core/runner.py ]; then
        pydir=$loc
        break
    fi
done
if [ -z "${pydir}" ]; then
    echo "Could not determine avocado installation location"
    exit 1
fi

PATCH_DIR="$PREFIX"/lib/daos/TESTING/ftest
# https://github.com/avocado-framework/avocado/pull/4345 fixed somewhere
# before 69.2
if grep "self.job.result_proxy.notify_progress(False)" \
    "$pydir"/avocado/core/runner.py; then
    echo "Applying patch avocado-job-result_proxy-reference-fix.patch"
    if ! cat < "$PATCH_DIR"/avocado-job-result_proxy-reference-fix.patch | \
      sudo patch -p1 -d "$pydir"; then
        echo "Failed to apply avocado PR-4345 patch"
        exit 1
    fi
fi
# https://github.com/avocado-framework/avocado/pull/2908 fixed in
# https://github.com/avocado-framework/avocado/pull/3076/
if ! grep "runner.timeout.process_died" "$pydir"/avocado/core/runner.py; then
    # this version of runner.py is older than 82.0
    if ! grep TIMEOUT_TEARDOWN "$pydir"/avocado/core/runner.py; then
        echo "Applying patch avocado-teardown-timeout.patch"
        if ! cat < "$PATCH_DIR"/avocado-teardown-timeout.patch | \
        sudo patch -p1 -d "$pydir"; then
            echo "Failed to apply avocado PR-3076 patch"
            exit 1
        fi
    fi
fi
# https://github.com/avocado-framework/avocado/pull/3154 - fixed somewhere
# before 69.2
if ! grep "def phase(self)" \
    "$pydir"/avocado/core/test.py; then
    echo "Applying patch avocado-report-test-phases-common.patch"
    if ! filterdiff -p1 -x selftests/* <                       \
        "$PATCH_DIR"/avocado-report-test-phases-common.patch | \
      sed -e '/selftests\/.*/d' |                              \
      sudo patch -p1 -d "$pydir"; then
        echo "Failed to apply avocado PR-3154 patch - common portion"
        exit 1
    fi
    if grep "^TEST_STATE_ATTRIBUTES = " "$pydir"/avocado/core/test.py; then
        echo "Applying patch avocado-report-test-phases-py3.patch"
        if ! cat < "$PATCH_DIR"/avocado-report-test-phases-py3.patch | \
          sudo patch -p1 -d "$pydir"; then
            echo "Failed to apply avocado PR-3154 patch - py3 portion"
            exit 1
        fi
    else
        echo "Applying patch avocado-report-test-phases-py2.patch"
        if ! cat < "$PATCH_DIR"/avocado-report-test-phases-py2.patch | \
          sudo patch -p1 -d "$pydir"; then
            echo "Failed to apply avocado PR-3154 patch - py2 portion"
            exit 1
        fi
    fi
fi
# apply fix for https://github.com/avocado-framework/avocado/issues/2908 - fixed
# somewhere before 69.2
if grep "TIMEOUT_TEST_INTERRUPTED" \
    "$pydir"/avocado/core/runner.py; then
        sudo ed <<EOF "$pydir"/avocado/core/runner.py
/TIMEOUT_TEST_INTERRUPTED/s/[0-9]*$/60/
wq
EOF
fi
# apply fix for https://jira.hpdd.intel.com/browse/DAOS-6756 for avocado 69.x -
# fixed somewhere before 82.0
if grep "TIMEOUT_PROCESS_DIED" \
    "$pydir"/avocado/core/runner.py; then
        sudo ed <<EOF "$pydir"/avocado/core/runner.py
/TIMEOUT_PROCESS_DIED/s/[0-9]*$/60/
wq
EOF
fi
# apply fix for https://github.com/avocado-framework/avocado/pull/2922 - fixed
# somewhere before 69.2
if grep "testsuite.setAttribute('name', 'avocado')" \
    "$pydir"/avocado/plugins/xunit.py; then
    sudo ed <<EOF "$pydir"/avocado/plugins/xunit.py
/testsuite.setAttribute('name', 'avocado')/s/'avocado'/os.path.basename(os.path.dirname(result.logfile))/
wq
EOF
fi
# Fix for bug to be filed upstream - fixed somewhere before 69.2
if grep "self\.job\.result_proxy\.notify_progress(False)" \
    "$pydir"/avocado/core/runner.py; then
    sudo ed <<EOF "$pydir"/avocado/core/runner.py
/self\.job\.result_proxy\.notify_progress(False)/d
wq
EOF
fi

pushd "$PREFIX"/lib/daos/TESTING/ftest

# make sure no lingering corefiles or junit files exist
rm -f core.* ./*_results.xml

# see if we just wanted to set up
if ${SETUP_ONLY:-false}; then
    exit 0
fi

export DAOS_APP_DIR=${DAOS_APP_DIR:-$DAOS_TEST_SHARED_DIR}

# check if slurm needs to be configured for soak
if [[ "${TEST_TAG_ARG}" =~ soak && "${STAGE_NAME}" =~ Hardware ]]; then
    if ! ./slurm_setup.py -d -c "$FIRST_NODE" -n "${TEST_NODES}" -s -i; then
        exit "${PIPESTATUS[0]}"
    fi

    if ! mkdir -p "${DAOS_APP_DIR}/soak/apps"; then
        exit "${PIPESTATUS[0]}"
    fi

    if ! cp -r /scratch/soak/apps/* "${DAOS_APP_DIR}/soak/apps/"; then
        exit "${PIPESTATUS[0]}"
    fi
fi

# need to increase the number of oopen files (on EL8 at least)
ulimit -n 4096

# Clean stale job results
if [ -d "${logs_prefix}/ftest/avocado/job-results" ]; then
    rm -rf "${logs_prefix}/ftest/avocado/job-results"
fi

# now run it!
# shellcheck disable=SC2086
export WITH_VALGRIND
export STAGE_NAME
export TEST_RPMS
export DAOS_BASE

node_args="-ts ${TEST_NODES}"
if [ "${STAGE_NAME}" == "Functional Hardware 24" ]; then
    # Currently the 'Functional Hardware 24' uses a cluster that has 8 hosts configured to run
    # daos engines and the remaining hosts are configured to be clients. Use separate -ts and -tc
    # launch.py arguments to ensure these hosts are not used for unintended role
    IFS=" " read -r -a test_node_list <<< "${TEST_NODES//,/ }"
    server_nodes=$(IFS=','; echo "${test_node_list[*]:0:8}")
    client_nodes=$(IFS=','; echo "${test_node_list[*]:8}")
    node_args="-ts ${server_nodes} -tc ${client_nodes}"
fi

# Run launch.py multiple times if a sequence of tags (tags separated by a '+') is specified
echo "TEST_TAG_ARG: ${TEST_TAG_ARG}"
IFS="+" read -r -a TEST_TAG_SPLIT <<< "${TEST_TAG_ARG}"
index=0
rc=0
echo "TEST_TAG_SPLIT: ${TEST_TAG_SPLIT[*]}"
for TEST_TAG_SEQ in "${TEST_TAG_SPLIT[*]}"; do
    IFS=" " read -r -a TAGS <<< "${TEST_TAG_SEQ}"
    if [ $index -eq 0 ]; then
        # First sequence of tags run with arguments provided by the user
        name=${STAGE_NAME}
    elif [ $index -eq 1 ]; then
        # Additional sequences of tags are ALWAYS run with servers using MD on SSD mode
        LAUNCH_OPT_ARGS="${LAUNCH_OPT_ARGS} --nvme=auto_md_on_ssd"
        name="${STAGE_NAME} MD on SSD"
    else
        echo "More than two sequences of functional test tags not supported"
        continue
    fi

    # shellcheck disable=SC2086,SC2090
    if ! ./launch.py --mode ci --name ${name} ${node_args} ${LAUNCH_OPT_ARGS} ${TAGS[*]}; then
        rc=$((rc|${PIPESTATUS[0]}))
    fi
done

exit $rc

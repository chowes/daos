# My personal WIP notes

# Get the 2.2.0 release repo
ALL_NODES=boro-[25-27,39]
clush -B -w $ALL_NODES 'sudo wget -O /etc/yum.repos.d/daos-packages-v2.2.repo https://packages.daos.io/v2.2/EL8/packages/x86_64/daos_packages.repo'
clush -B -w $ALL_NODES 'sudo wget -O /etc/yum.repos.d/daos-packages-v2.3.108.repo https://packages.daos.io/private/v2.3.108/EL8/packages/x86_64/daos_packages.repo'

# To get local avocado to work with RPM install
cp /usr/lib/daos/.build_vars.* /home/dbohning/daos/install/lib/daos/

# Copy repo files from local to all nodes
ALL_NODES=boro-[25-27,39]
clush -B -w $ALL_NODES "sudo cp /home/dbohning/repos/* /etc/yum.repos.d/"

# Install 2.2.0 release
ALL_NODES=boro-[25-27,39]
VERSION="2.2.0-4.el8"
clush -B -w $ALL_NODES "sudo systemctl stop daos_agent; sudo systemctl stop daos_server;"; \
clush -B -w $ALL_NODES "sudo dnf remove -y daos"; \
clush -B -w $ALL_NODES "sudo dnf install -y daos-server-tests-${VERSION} daos-tests-${VERSION} ior"; \
clush -B -w $ALL_NODES "rpm -qa | grep daos | sort"; \
cp /usr/lib/daos/.build_vars.* /home/dbohning/daos/install/lib/daos/

# Rebuild just ftest locally
~/bin/daos_rebuild_ftest

# Run launch.py
DAOS_CLIENTS=boro-25
DAOS_SERVERS=boro-[26-27,39]
cd ~/daos/install/lib/daos/TESTING/ftest/;  \
cp /usr/lib/daos/.build_vars.* /home/dbohning/daos/install/lib/daos/;  \
./launch.py --provider "ofi+tcp;ofi_rxm" -aro -tc $DAOS_CLIENTS -ts $DAOS_SERVERS test_upgrade_downgrade;

# Copy configs from ftest to nodes
DAOS_CLIENTS=boro-25
DAOS_SERVERS=boro-[26-27,39]
clush -w $DAOS_CLIENTS 'sudo cp /home/dbohning/avocado/job-results/latest/daos_configs.boro-25/daos_agent.yml /etc/daos/daos_agent.yml'; \
clush -w $DAOS_CLIENTS 'sudo cp /home/dbohning/avocado/job-results/latest/daos_configs.boro-25/daos_control.yml /etc/daos/daos_control.yml'; \
clush -w $DAOS_CLIENTS 'sudo cp /home/dbohning/avocado/job-results/latest/daos_configs.boro-25/daos_server.yml /etc/daos/daos_server.yml'; \
clush -w $DAOS_SERVERS 'sudo cp /home/dbohning/avocado/job-results/latest/daos_configs.boro-25/daos_agent.yml /etc/daos/daos_agent.yml'; \
clush -w $DAOS_SERVERS 'sudo cp /home/dbohning/avocado/job-results/latest/daos_configs.boro-25/daos_control.yml /etc/daos/daos_control.yml'; \
clush -w $DAOS_SERVERS 'sudo cp /home/dbohning/avocado/job-results/latest/daos_configs.boro-25/daos_server.yml /etc/daos/daos_server.yml';

# Compare rpms between two nodes
NODE1=boro-25
NODE2=boro-26
diff <(clush -B -w $NODE1 'rpm -qa | sort' 2>&1) <(clush -B -w $NODE2 'rpm -qa | sort' 2>&1)

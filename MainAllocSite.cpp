#include "llvm/IR/Instructions.h"

#include "AllocSite.h"
#include "Config.h"
#include "ParseIR.h"

using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    vector<string> inputFilenames = parseInput("input");
    std::ostream out(0);
    vector<Module *> moduleList = parseIRFilesMultithread(inputFilenames, out);
    GlobalState glbState(moduleList, out);

    multimap<StructType *, CallInst *> rets[2]; // rets[0] is general, rets[1] is special
    string outputPrefixes[2] {"[general] ", "[special] "};
    getAllocSites(glbState, rets[0], rets[1]);
    for (auto i = 0; i < 2; ++i)
    {
        for (auto it = rets[i].begin(); it != rets[i].end(); ++it) {
            string objName = it->first->getStructName().str();
            CallInst *callInst = it->second;
            Function *callee = callInst->getCalledFunction();
            string calleeName = callee->getName().str();
            const DataLayout &dataLayout = callInst->getModule()->getDataLayout();
            uint64_t size = 0;
            if (!it->first->isOpaque())
            {
                size = dataLayout.getTypeAllocSize(it->first);
            }
            cout << outputPrefixes[i] << objName << " [size] " << size << " [id] " << (uint64_t) it->first << " allocated by " << calleeName << " in " << callInst->getFunction()->getName().str() << endl;
        }
    }

    // search elastic object types
    string elasticObjectNames[]{"struct.ipv6_opt_hdr", "struct.sock_fprog_kern", "struct.policy_load_memory", "struct.ldt_struct", "struct.ip_options", "struct.iovec", "struct.cfg80211_pkt_pattern", "struct.user_key_payload", "struct.xfrm_replay_state_esn", "struct.ip_sf_socklist", "struct.cache_reader", "struct.tc_cookie", "struct.cfg80211_bss_ies", "struct.sg_header", "struct.inotify_event_info", "struct.fb_cmap_user", "struct.cache_request", "struct.msg_msg", "struct.fname", "struct.ieee80211_mgd_auth_data", "struct.tcp_fastopen_context", "struct.request_key_auth", "struct.xfrm_algo_auth", "struct.cfg80211_wowlan_tcp", "struct.xfrm_algo", "struct.xfrm_algo_aead", "struct.cfg80211_scan_request", "struct.mon_reader_bin", "struct.cfg80211_sched_scan_request", "struct.mon_reader_text", "struct.station_info", "struct.ext4_dir_entry_2", "struct.xfrm_policy", "struct.fb_info", "struct.audit_rule_data", "struct.n_tty_data", "struct.ax25_dev"};
    for (string name : elasticObjectNames)
    {
        glbState.searchTypes(name.c_str());
    }
}

// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-8888 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

//
// Main network
//

// Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)

{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.

    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x13;
        pchMessageStart[1] = 0x1b;
        pchMessageStart[2] = 0x21;
        pchMessageStart[3] = 0x5c;
        nDefaultPort = 9952;
        nRPCPort = 9953;
    bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20);

        // Build the genesis block.
    const char* pszTimestamp = "Master Wayawolfcoin Canis Lupus The 3rd April 2024 "; //
        std::vector<CTxIn> vin;
        vin.resize(1);
        vin[0].scriptSig = CScript() << 0 << CBigNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        std::vector<CTxOut> vout;
        vout.resize(1);
        vout[0].SetEmpty();
        CTransaction txNew(1, 1713131471, vin, vout, 0); // Sunday, April 14, 2024
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1713131471;
        genesis.nBits    = bnProofOfWorkLimit.GetCompact();
		genesis.nNonce   = 1835781;

        hashGenesisBlock = genesis.GetHash();
		bool creategenesis=false;
					if ( creategenesis)
					{
						LogPrintf("Searching for genesis block...\n");
						// This will figure out a valid hash and Nonce if you're
						// creating a different genesis block:
						uint256 hashTarget = CBigNum().SetCompact(genesis.nBits).getuint256();

						while(creategenesis)
						{
							LogPrintf("nonce %08X: hash = %s (target = %s)\n", genesis.nNonce, genesis.GetHash().ToString().c_str(), hashTarget.ToString().c_str());

							printf("true? %b",CheckProofOfWork(genesis.GetHash(),genesis.nBits));
		//if satisfies work limit break while loop and print details
		 if (CheckProofOfWork(genesis.GetHash(),genesis.nBits))
								break;

		//if all 0 just print and increment
							if ((genesis.nNonce & 0xFFF) == 0)
							{
								LogPrintf("nonce %08X: hash = %s (target = %s)\n", genesis.nNonce, genesis.GetHash().ToString().c_str(), hashTarget.ToString().c_str());
						   }

							++genesis.nNonce;
		//if we wrapped over big int no hash will ever work increment time start by a millisecond and continue
							if (genesis.nNonce == 0)
							{
								LogPrintf("NONCE WRAPPED, incrementing time\n");
								++genesis.nTime;
							}
						}
						LogPrintf("merkleRoot = %s",genesis.hashMerkleRoot.ToString().c_str());
						LogPrintf("block.nTime = %u \n", genesis.nTime);
						LogPrintf("block.nNonce = %u \n", genesis.nNonce);
						LogPrintf("block.GetHash = %s\n", genesis.GetHash().ToString().c_str());
		 }
		assert(hashGenesisBlock == uint256("0x00000430883f8c219da241e50bcfa8b745b4416465ebb4e07b71106d98b8c60d"));
		assert(genesis.hashMerkleRoot ==uint256("0xb3acbbcf7f55c83e748597de3335289c7fd0e631805b24d5c10fa174004996dc"));

        /** DEPRICATED IN QT 5.6+ (To compile on Qt5.5.1 and lower uncomment  */
        /*
        base58Prefixes[PUBKEY_ADDRESS] = list_of(73);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(42);
        base58Prefixes[SECRET_KEY] =     list_of(133);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0xAD)(0x1B)(0x12)(0xA4);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0xE1)(0xA3)(0x2B)(0x3E);
        */
        /** REQUIRED IN QT 5.6+  (To compile on Qt5.5.1 and lower comment out below) */
    base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 73);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 42);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1, 133);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0xAD)(0x1B)(0x12)(0xA4).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0xE1)(0xA3)(0x2B)(0x3E).convert_to_container<std::vector<unsigned char> >();

        // WayaWolfCoin DNS seed servers
        vSeeds.push_back(CDNSSeedData("DNSSeeder1", "seed1.wayawolfcoin.com"));   // dev-seed #1
        vSeeds.push_back(CDNSSeedData("DNSSeeder2", "seed2.wayawolfcoin.com"));   // dev-seed #2
        vSeeds.push_back(CDNSSeedData("DNSSeeder3", "seed3.wayawolfcoin.com"));   // dev-seed #3
        vSeeds.push_back(CDNSSeedData("DNSSeeder4", "ww-001.nerdlabs001.com"));   // labs.com #1
        vSeeds.push_back(CDNSSeedData("DNSSeeder5", "ww-002.nerdlabs001.com"));   // labs.com #2
        vSeeds.push_back(CDNSSeedData("DNSSeeder6", "ww-003.nerdlabs001.com"));   // labs.com #3
        vSeeds.push_back(CDNSSeedData("DNSSeeder0", "ww-blocks.nerdlabs.dev"));   // seed-explorer 2024

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

	nLastPOWBlock = 0x7fffffff;
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }

protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet
//

class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x25;
        pchMessageStart[1] = 0x5a;
        pchMessageStart[2] = 0xd6;
        pchMessageStart[3] = 0x21;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 8);
        nDefaultPort = 4913;
        nRPCPort = 4912;
	//nDefaultPort = 1600;
        //nRPCPort = 1601;
        strDataDir = "testnet";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 612;
        
        hashGenesisBlock = genesis.GetHash();

	 //   assert(hashGenesisBlock == uint256("0xc28f948927c9f18dba652b887c962dcdd6057050b943a83011bcf88ee3ded3fe"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,117);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,115);
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1,63);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x14)(0x1a)(0x2c)(0x24).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x1f)(0xfa)(0x1e)(0x3e).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        nLastPOWBlock = 0x7fffffff;
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;

        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {

    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}

hb_filt2 = zeros(1,43);
hb_filt2([1 3 5 7 9 11 13 15 17 19 21]) = [12 -32 72 -140 252 -422 682 -1086 1778 -3284 10364];
hb_filt2([23 25 27 29 31 33 35 37 39 41 43]) = hb_filt2(fliplr([1 3 5 7 9 11 13 15 17 19 21]));
hb_filt2(22) = 16384;
hb_filt2 = hb_filt2./32768;

addpath('../util');
addpath('./mcode_blocks');
addpath('./blackboxes');

PLCP_Preamble = PLCP_Preamble_gen;

%%
%xlLoadChipScopeData('cs_capt/wlan_cs_capt_9.prn'); cs_interp = 8; cs_start = 1;
%xlLoadChipScopeData('cs_capt/wlan_cs_capt_10.prn'); cs_interp = 8; cs_start = 1;
%xlLoadChipScopeData('cs_capt/wlan_cs_capt_5.prn'); cs_interp = 1; cs_start = 1;
%xlLoadChipScopeData('cs_capt/wlan_cs_capt_12_badRxLen.prn'); cs_interp = 8; cs_start = 1;
%xlLoadChipScopeData('cs_capt/dsss_cs_capt_3.prn'); cs_interp = 1; cs_start = 1; %DSSS

%xlLoadChipScopeData('cs_capt/wlan_cs_capt_23_goodFCS_18M.prn'); cs_interp = 1; cs_start = 1; cs_end = 1.8e3; %no agc
%samps1 = complex(ADC_I(cs_start:cs_interp:cs_end), ADC_Q(cs_start:cs_interp:cs_end));

xlLoadChipScopeData('cs_capt/wlan_cs_capt_37_badSignal_12M.prn'); cs_interp = 8; cs_start = 1; cs_end = 11e3;
samps2 = complex(ADC_I(cs_start:cs_interp:cs_end), ADC_Q(cs_start:cs_interp:cs_end));

%samps2 = complex(ADC_I, ADC_Q);
%raw_rx_IQ_Valid.time = [];
%raw_rx_IQ_Valid.signals.values = IQ_Valid;

%payload_vec = [zeros(500, 1); samps2; zeros(50,1);];
payload_vec = [zeros(500, 1); samps2; zeros(50,1);zeros(500, 1); samps2; zeros(50,1);];
simtime = 8*length(payload_vec) + 500;

%One CS capture
%payload_vec = [zeros(25,1); complex(ADC_I(cs_start:cs_interp:cs_end), ADC_Q(cs_start:cs_interp:cs_end));];

%wlan_tx output
%load wlan_tx_out_34PB_Q12.mat
%load wlan_tx_out_34PB_Q34.mat
%payload_vec = [zeros(100,1); wlan_tx_out; zeros(200,1); wlan_tx_out; zeros(1000,1)];

%DSSS capt
%load dsss_capt_v0.mat; t_capt = 1.25e4:3.27e4;
%rx_raw = 5*rx_IQ(t_capt).';
%raw_rx_dec = filter(hb_filt2, 1, rx_raw);
%raw_rx_dec = raw_rx_dec(1:2:end);
%payload_vec = [zeros(100,1); raw_rx_dec.'; zeros(1000,1)];

%Two packets
%payload_vec = [zeros(125,1); complex(ADC_IA(cs_start:cs_interp:end), ADC_QA(cs_start:cs_interp:end));  complex(ADC_IB(cs_start:cs_interp:end), ADC_QB(cs_start:cs_interp:end))];

%Perfect preamble, fuzzed first pkt
%payload_vec = [zeros(1,125) 2.*repmat(PLCP_Preamble.STS_t, 1, 10) PLCP_Preamble.LTS_t(33:64) 1*PLCP_Preamble.LTS_t 1*PLCP_Preamble.LTS_t 0.1.*complex(randn(1,1000),randn(1,1000)) zeros(1,200) complex(ADC_IA(cs_start:cs_interp:end), ADC_QA(cs_start:cs_interp:end)).'].';

raw_rx_I.time = [];
raw_rx_Q.time = [];
%raw_rx_I.signals.values = -1*ones(1,length(payload_vec)).';%-1*[repmat([1 -1], 1, length(payload_vec))].';%real(payload_vec);
raw_rx_I.signals.values = real(payload_vec);
raw_rx_Q.signals.values = imag(payload_vec);


%%
MAX_NUM_SC = 64;
MAX_CP_LEN = 32;
MAX_NUM_SAMPS = 50e3;
MAX_NUM_SYMS = 600;
MAX_NUM_BYTES = 4096;
MAX_NCBPS = 288;


PHY_CONFIG_NUM_SC = 64;
PHY_CONFIG_CP_LEN = 16;
PHY_CONFIG_FFT_OFFSET = 7;% 5 = no CP samples into FFT (5=zero actual offset)
PHY_CONFIG_CFO_EST_OFFSET = 0;
PHY_CONFIG_FFT_SCALING = bin2dec('010110');

% Long correlation init
%Fix3_0 version of longSym
longCorr_coef_nbits = 3;
longCorr_coef_bp = 0;
LTS_corr = fliplr(PLCP_Preamble.LTS_t./max(abs(PLCP_Preamble.LTS_t)));

%longCorr_coef_i = [-4*real(LTS_corr)];
%longCorr_coef_q = [4*imag(LTS_corr)];
longCorr_coef_i = [8*real(LTS_corr)];
longCorr_coef_q = [-8*imag(LTS_corr)];
%longCorr_coef_i((longCorr_coef_i < -7.5)) = -7;
%longCorr_coef_q((longCorr_coef_q < -7.5)) = -7;

%LC TESTING ONLY
%longCorr_coef_i = ones(1,length(longCorr_coef_i));

%long_cor_acc_n_bits = 6;
long_cor_acc_n_bits = 6 * 2;

%FFT Ctrl config
n_bits_preFFT_sampBuff = ceil(log2(4*MAX_NUM_SC));

%Training symbol ROM
train_sym_f = sign(PLCP_Preamble.LTS_f);

%Data-bearing subcarrier map
sc_ind_data = [2:7 9:21 23:27 39:43 45:57 59:64];
sc_data_map = zeros(1,MAX_NUM_SC);
sc_data_map(sc_ind_data) = 1;
sc_data_sym_map = MAX_NUM_SC*ones(1,MAX_NUM_SC); %use MAX_NUM_SC to flag empty subcarriers
sc_data_sym_map(sc_ind_data) = fftshift(0:47);

%Register init
PHY_MIN_PKT_LEN = 14;

PHY_CONFIG_LTS_CORR_THRESH = 3e4;
PHY_CONFIG_LTS_CORR_TIMEOUT = 250;%150;%*2 in hardware

PHY_CONFIG_PKT_DET_CORR_THRESH = 100;%90;
PHY_CONFIG_PKT_DET_ENERGY_THRESH = 250;%7; %CHANGE BACK TO 250!
PHY_CONFIG_PKT_DET_MIN_DURR = 4;
PHY_CONFIG_PKT_DET_RESET_EXT_DUR = hex2dec('3F');

PHY_CONFIG_RSSI_SUM_LEN = 16;
CS_CONFIG_CS_RSSI_THRESH = 500 * PHY_CONFIG_RSSI_SUM_LEN;
CS_CONFIG_POSTRX_EXTENSION = 120; %6usec as 120 20MHz samples

SOFT_DEMAP_SCALE_QPSK = 6;
SOFT_DEMAP_SCALE_16QAM = 16;
SOFT_DEMAP_SCALE_64QAM = 16;

REG_RX_PktDet_AutoCorr_Config = ...
    2^0  *  (PHY_CONFIG_PKT_DET_CORR_THRESH) +...%b[7:0]
    2^8  *  (PHY_CONFIG_PKT_DET_ENERGY_THRESH) +...%b[19:8]
    2^20 *  (PHY_CONFIG_PKT_DET_MIN_DURR) +...%b[25:20]
    2^26 *  (PHY_CONFIG_PKT_DET_RESET_EXT_DUR) + ...%b[31:26]
    0;

REG_RX_LTS_Corr_Config = ...
    2^0  *  (PHY_CONFIG_LTS_CORR_THRESH) +... %b[17:0]
    2^24 *  (PHY_CONFIG_LTS_CORR_TIMEOUT) +... %b[31:24]
    0;

REG_RX_FFT_Config = ...
    2^0  * (PHY_CONFIG_NUM_SC) +...  %b[7:0]
    2^8  * (PHY_CONFIG_CP_LEN) +...  %b[15:8]
    2^16 * (PHY_CONFIG_FFT_OFFSET) +...  %b[23:16]
    2^24 * (PHY_CONFIG_FFT_SCALING) + ... b[29:24]
    0;

REG_RX_Control = ...
    2^0 * 0 + ... %b[0]: Global Reset
    2^1 * 0 + ... %b[1]: Pkt done latch reset
    0;

REG_RX_Config = ...
    2^0  * 1 + ...; %DSSS RX EN
    2^1  * 1 + ...; %DSSS RX Blocks
    2^2  * 1 + ...; %Swap pkt buf byte order
    2^3  * 0 + ...; %Block Rx on FCS bad
    0;

REG_RX_DSSS_RX_CONFIG = ...
    2^0 * (hex2dec('400')) + ... %b[15:0]: Code Thresh UFix16_8
    2^16 * 200 + ... %b[23:16]: Bit count timeout
    2^24 * 6 + ... %b[29:24]: Depsread delay (UFix5_0)
    2^29 * 5; %b[31:29]: Length pad (in bytes)

REG_RX_PKTDET_RSSI_CONFIG = ...
    2^0 * (PHY_CONFIG_RSSI_SUM_LEN) + ... %b[4:0]: RSSI sum len
    2^5 * (500*16) + ... %b[19:5]: RSSI thresh
    2^20 * (5) + ... %b[24:20]: Min duration
    0;

REG_RX_CCA_CONFIG = ...
    2^0 *  (CS_CONFIG_CS_RSSI_THRESH) + ... %b[15:0]
    2^16 * (CS_CONFIG_POSTRX_EXTENSION) + ... %b[23:16]
    0;



REG_RX_FEC_Config = ...
    2^0  * (SOFT_DEMAP_SCALE_QPSK) + ...
    2^5  * (SOFT_DEMAP_SCALE_16QAM) + ...
    2^10 * (SOFT_DEMAP_SCALE_64QAM) + ...
    0;

%%%%%%%%%%%
% DSSS Rx
barker_seq20 = [1.29007 1.04043 1.20873 -0.32809 -1.55859 0.69252 1.62682 0.54184 1.06449 1.40040 0.11423 -1.20708 -1.26002 -0.54425 -1.31058 -1.27990 1.38940 0.97934 -1.65552 -0.38597];
barker_seq20 = 0.95 * circshift(barker_seq20, [0 4]) ./ max(abs(barker_seq20));


%%
bit_scrambler_lfsr = ones(1,7);
bit_scrambler_lfsr_states = zeros(127, 7);
scr = zeros(1,127);
for ii=1:127
    bit_scrambler_lfsr_states(ii, :) = bit_scrambler_lfsr;

    x = xor(bit_scrambler_lfsr(4), bit_scrambler_lfsr(7));
    bit_scrambler_lfsr = [x bit_scrambler_lfsr(1:6)];

    scr(ii) = x;
end
bit_scrambler_lfsr_bytes = bi2de(reshape(repmat(scr, 1, 8), 8, 127)', 'left-msb');
%%

scr = [scr scr(1:10)];
scr_ind_rev = zeros(1,128);
for ii=1:127
%    scr_ind(ii) = bi2de(scr(ii:ii+6));
    scr_ind_rev(1 + bi2de(scr(ii:ii+6))) = ii - 1;
end
clear scr x bit_scrambler_lfsr ii

%% Cyclic Redundancy Check parameters
CRCPolynomial32 = hex2dec('04c11db7'); %CRC-32
CRC_Table32 = CRC_table_gen(CRCPolynomial32, 32);


%Calculate interleaving vectors
NCBPS_BPSK = 48;
NCBPS_QPSK = 96;
NCBPS_16QAM = 192;
NCBPS_64QAM = 288;

%% BPSK
N_CBPS = 48;
N_BPSC = 1;
s = max(N_BPSC/2, 1);

%Interleaver (k=src bit index -> j=dest bit index)
k = 0:N_CBPS-1;
i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
%BPSK doesn't need j

interleave_BPSK = i;
clear N_CBPS N_BPSC s k i

%% QPSK
N_CBPS = 96;
N_BPSC = 2;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(16*i/N_CBPS)), s);
interleave_QPSK = j;
clear N_CBPS N_BPSC s k i j

%% 16-QAM
N_CBPS = 192;
N_BPSC = 4;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(16*i/N_CBPS)), s);
interleave_16QAM = j;
clear N_CBPS N_BPSC s k i j

%FIXME:
interleave_64QAM = interleave_16QAM;

%FFT Shift
interleave_BPSK = mod(interleave_BPSK + (NCBPS_BPSK/2), NCBPS_BPSK);
interleave_QPSK = mod(interleave_QPSK + (NCBPS_QPSK/2), NCBPS_QPSK);
interleave_16QAM = mod(interleave_16QAM + (NCBPS_16QAM/2), NCBPS_16QAM);
interleave_64QAM = mod(interleave_64QAM + (NCBPS_64QAM/2), NCBPS_64QAM);

deinterleave_ROM = [];
deinterleave_ROM = [deinterleave_ROM interleave_BPSK zeros(1, 512-length(interleave_BPSK))];
deinterleave_ROM = [deinterleave_ROM interleave_QPSK zeros(1, 512-length(interleave_QPSK))];
deinterleave_ROM = [deinterleave_ROM interleave_16QAM zeros(1, 512-length(interleave_16QAM))];
deinterleave_ROM = [deinterleave_ROM interleave_64QAM zeros(1, 512-length(interleave_64QAM))];



hb_filt2 = zeros(1,43);
hb_filt2([1 3 5 7 9 11 13 15 17 19 21]) = [12 -32 72 -140 252 -422 682 -1086 1778 -3284 10364];
hb_filt2([23 25 27 29 31 33 35 37 39 41 43]) = hb_filt2(fliplr([1 3 5 7 9 11 13 15 17 19 21]));
hb_filt2(22) = 16384;
hb_filt2 = hb_filt2./32768;

addpath('./util');
addpath('./mcode_blocks');
addpath('./blackboxes');

PLCP_Preamble = PLCP_Preamble_gen;

%%
%xlLoadChipScopeData('cs_capt/wlan_cs_capt_81_64Q23.prn'); cs_interp = 1; cs_start = 1300; cs_end = length(ADC_I);
%samps2 = complex(ADC_I(cs_start:cs_interp:cs_end), ADC_Q(cs_start:cs_interp:cs_end));
%payload_vec = [samps2; zeros(1000,1);];
%paylod_vec_samp_time = 8;

samps_rssi_avg.time = [];
samps_rssi_avg.signals.values = 0;%RSSI(cs_start:end);%RSSI is delayed by 2 T=16 samples before CS ILA

samps_pkt_det.time = [];
samps_pkt_det.signals.values = 0;%Pkt_Det2(cs_start:end);

samps_iq_valid.time = [];
samps_iq_valid.signals.values = 0;%ADC_IQ_Valid2(cs_start:end);


%wlan_tx output %wlan_tx_out_74PB_64Q34 is worst case for last samp -> decode latency
%load('rx_sigs/wlan_tx_out_81B_Q12.mat'); tx_sig_t = [1:length(wlan_tx_out)];
%load('rx_sigs/wlan_tx_out_120PB_64Q23.mat'); tx_sig_t = [1:length(wlan_tx_out)];
%load('rx_sigs/wlan_tx_out_34PB_Q34.mat'); tx_sig_t = [1:length(wlan_tx_out)];
%load('rx_sigs/wlan_tx_out_3pkts_16Q34.mat'); tx_sig_t = [1:length(wlan_tx_out)];
%load('rx_sigs/wlan_tx_out_1240PB_16Q12.mat'); tx_sig_t = 1:length(wlan_tx_out);
%load('rx_sigs/wlan_tx_out_74PB_64Q34.mat'); tx_sig_t = [1:length(wlan_tx_out)];
load('rx_sigs/wlan_tx_out_81B_64Q34.mat'); tx_sig_t = [1:length(wlan_tx_out)];
%load('rx_sigs/wlan_tx_out_34PB_Q34.mat'); tx_sig_t = [1:length(wlan_tx_out)]; %%%FCS BAD IN RX SIM?!

payload_vec = [zeros(50,1); wlan_tx_out(tx_sig_t); zeros(100,1);];
%cfo = exp(1i*2*pi*(5e-4)*(0:length(payload_vec)-1)).';
%payload_vec = payload_vec .* cfo;

%payload_vec((50+32+160+160+4)+[1:70]) = 0;%Force SIGNAL error in first pkt
%payload_vec = [zeros(50,1); wlan_tx_out(tx_sig_t); zeros(10,1); wlan_tx_out(tx_sig_t); zeros(100,1)];
paylod_vec_samp_time = 8;

simtime = 8*length(payload_vec) + 500;
raw_rx_I.time = [];
raw_rx_Q.time = [];
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
PHY_CONFIG_FFT_SCALING = bin2dec('000101');

% Long correlation init
%Fix3_0 version of longSym
longCorr_coef_nbits = 3;
longCorr_coef_bp = 0;
LTS_corr = fliplr(conj(PLCP_Preamble.LTS_t./max(abs(PLCP_Preamble.LTS_t))));

longCorr_coef_i = [3*real(LTS_corr)];
longCorr_coef_q = [3*imag(LTS_corr)];

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
PHY_CONFIG_RSSI_SUM_LEN = 8;

PHY_MIN_PKT_LEN = 14;

PHY_CONFIG_LTS_CORR_THRESH_LOWSNR = 12500;
PHY_CONFIG_LTS_CORR_THRESH_HIGHSNR = 17000;
PHY_CONFIG_LTS_CORR_RSSI_THRESH = PHY_CONFIG_RSSI_SUM_LEN*400;

PHY_CONFIG_LTS_CORR_TIMEOUT = 250;%150;%*2 in hardware

PHY_CONFIG_PKT_DET_CORR_THRESH = 200;%90;
PHY_CONFIG_PKT_DET_ENERGY_THRESH = 0*250;%Set to 0 for Rx sim of sim Tx waveform
PHY_CONFIG_PKT_DET_MIN_DURR = 4;
PHY_CONFIG_PKT_DET_RESET_EXT_DUR = hex2dec('3F');

CS_CONFIG_CS_RSSI_THRESH = 300 * PHY_CONFIG_RSSI_SUM_LEN;
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

REG_RX_LTS_Corr_Thresh = ...
    2^0  *  (PHY_CONFIG_LTS_CORR_THRESH_LOWSNR) +... %b[15:0]
    2^16  * (PHY_CONFIG_LTS_CORR_THRESH_HIGHSNR) +... %b[31:16]
    0;

REG_RX_LTS_Corr_Confg = ...
    2^0 *  (PHY_CONFIG_LTS_CORR_TIMEOUT) + ... %b[7:0]
    2^8 *  (PHY_CONFIG_LTS_CORR_RSSI_THRESH) + ... %b[23:8]
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
    2^0  * 1 + ... %DSSS RX EN
    2^1  * 1 + ... %Block inputs on INVALID input
    2^2  * 1 + ... %Swap pkt buf byte order
    2^3  * 0 + ... %Swap order of chan est u32 writes
    2^4  * 1 + ... %Allow DSSS Rx to keep AGC locked
    2^5  * 1 + ... %Bypass CFO est/correction
    2^6  * 1 + ... %Enable chan est recording to pkt buf
    2^7  * 1 + ... %Enable switching diversity
    2^8  * 0 + ... %Force selection of Ant B
    0;

REG_RX_DSSS_RX_CONFIG = ...
    2^0 * (hex2dec('600')) + ... %b[15:0]: Code Thresh UFix16_8
    2^16 * 200 + ... %b[23:16]: Bit count timeout
    2^24 * 5 + ... %b[29:24]: Depsread delay (UFix5_0)
    2^29 * 5; %b[31:29]: Length pad (in bytes)

REG_RX_PKTDET_RSSI_CONFIG = ...
    2^0 * (PHY_CONFIG_RSSI_SUM_LEN) + ... %b[4:0]: RSSI sum len
    2^5 * (300*8) + ... %b[19:5]: RSSI thresh
    2^20 * (4) + ... %b[24:20]: Min duration
    0;

REG_RX_CCA_CONFIG = ...
    2^0 *  (CS_CONFIG_CS_RSSI_THRESH) + ... %b[15:0]
    2^16 * (CS_CONFIG_POSTRX_EXTENSION) + ... %b[23:16]
    0;

REG_RX_PktBuf_Sel = ...
    2^0 *  0 + ... %b[3:0]: OFDM Pkt Buf
    2^8 *  0 + ... %b[11:8]: DSSS Pkt Buf
    2^16 * 0 + ... %b[23:16]: Pkt buf offset for Rx bytes (u64 words)
    2^24 * (8/8) + ... %b[31:24]: Pkt buf offset for chan est (u64 words)
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

NDBPS_BPSK12 = 24;
NDBPS_BPSK34 = 36;
NDBPS_QPSK12 = 48;
NDBPS_QPSK34 = 72;
NDBPS_16QAM12 = 96;
NDBPS_16QAM34 = 144;
NDBPS_64QAM23 = 192;
NDBPS_64QAM34 = 216;

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

%% 64-QAM
N_CBPS = 288;
N_BPSC = 6;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(16*i/N_CBPS)), s);
interleave_64QAM = j;
clear N_CBPS N_BPSC s k i j

%FFT Shift
interleave_BPSK = mod(interleave_BPSK + (NCBPS_BPSK/2), NCBPS_BPSK);
interleave_QPSK = mod(interleave_QPSK + (NCBPS_QPSK/2), NCBPS_QPSK);
interleave_16QAM = mod(interleave_16QAM + (NCBPS_16QAM/2), NCBPS_16QAM);
interleave_64QAM = mod(interleave_64QAM + (NCBPS_64QAM/2), NCBPS_64QAM);

%%
deinterleave_ROM = [];
deinterleave_ROM = [deinterleave_ROM interleave_BPSK zeros(1, 512-length(interleave_BPSK))];
deinterleave_ROM = [deinterleave_ROM interleave_QPSK zeros(1, 512-length(interleave_QPSK))];
deinterleave_ROM = [deinterleave_ROM interleave_16QAM zeros(1, 512-length(interleave_16QAM))];
deinterleave_ROM = [deinterleave_ROM interleave_64QAM zeros(1, 512-length(interleave_64QAM))];

%Transform deint ROM vector to pairwise-packed values, so two can be read per cycle from single port ROM
%deinterleave_ROM = deinterleave_ROM .* repmat([1 2^9], 1, length(deinterleave_ROM)/2);
%deinterleave_ROM = sum(reshape(deinterleave_ROM, 2, length(deinterleave_ROM)/2));
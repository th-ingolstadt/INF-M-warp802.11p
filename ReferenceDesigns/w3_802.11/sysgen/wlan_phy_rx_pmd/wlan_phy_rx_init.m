% Mango 802.11 Reference Design
% WLAN PHY Rx Init script
% Copyright 2017 Mango Communications
% Distributed under the Mango Research License:
% http://mangocomm.com/802.11/license

%clear all
addpath('./util');
addpath('./mcode_blocks');
addpath('./blackboxes');

%% Define an input signal for simulation
% Skip this if running a multi-sim test with the rx_sim_test script
if(~exist('sim_many_waveform_mode','var'))
    % Ensure previously-defined waveforms are cleared
    clear wlan_tx_out;
    clear sig;
    clear tx_sig;
    clear ADC_I ADC_Q;
    clear sim_sig;
    
    %PHY debugging with ChipScope captures of I/Q
    % ChipScope waveforms must be saved in ASCII format with (at least) ADC_I and ADC_Q signals
    %xlLoadChipScopeData('dsss_start_end_v0.prn'); cs_interp = 1; cs_start = 11750; cs_end = length(ADC_I);
    %sim_sig = complex(ADC_I([cs_start:cs_interp:cs_end]), ADC_Q(cs_start:cs_interp:cs_end));

    if 1
    %Output of PHY Tx simulation
    % .mat files from Tx PHY sim store I/Q signal in 'wlan_tx_out' variable
    load('rx_sigs/wlan_tx_NONHT_MCS4_52B.mat');wlan_tx_out = wlan_tx_out.';
    %load('rx_sigs/rx_11b_1Mb_then_3_5p5Mb.mat');wlan_tx_out = rx_iq(93370:2:end).';
    %load('dsss_tx_sim_0.mat'); wlan_tx_out = 0.4 .* pkt_wvfm;
    
    %p = util_dsss_tx(20, 1:10);
    %wlan_tx_out = 0.35 * p.wvfm / max(real(p.wvfm));
    %wlan_tx_out = [zeros(1,10) wlan_tx_out];
    
    %load DSSS_beacons_high_snr.mat
    %rx_iq_full = rx_iq;
    %rx_iq = rx_iq_full(2.133e5 : 2.477e5);
    %wlan_tx_out = rx_iq_full(6.231e5 : 2: 6.576e5).';
    
    
    %CFO
    %wlan_tx_out = wlan_tx_out .* exp(j*2*pi*-1e-4*(0:length(wlan_tx_out)-1));

    %sig = wlan_tx_out;
    %noise = 6e-2*complex(randn(size(wlan_tx_out)),randn(size(wlan_tx_out)));
    %wlan_tx_out = [zeros(1,0) sig + noise];
    %wlan_tx_out = sig;
    

	%Phase offset
    %wlan_tx_out = wlan_tx_out .* exp(-j*pi/4);
        
    %AWGN
    %wlan_tx_out = wlan_tx_out + 1e-2*complex(randn(1,length(wlan_tx_out)), randn(1,length(wlan_tx_out)));
    
    % 1-pkt waveform
    sim_sig = wlan_tx_out(1:end).';
    
    % 2-pkt waveform
    %sim_sig = [wlan_tx_out(1:end).'; zeros(200,1); wlan_tx_out(1:end).'];
    end
end


%Define the simulation paramters - waveform, sample rate, sim duration, etc
rx_sim = struct();
rx_sim.waveform_RFA.time = [];
rx_sim.waveform_RFA.signals.values = [zeros(50,1); sim_sig; zeros(500,1); ];
rx_sim.samp_rate = 20; %Must be in [10 20 40]
rx_sim.sim_time = (160 / rx_sim.samp_rate) * length(rx_sim.waveform_RFA.signals.values) + 500;

%%

%Fixed PHY parameters - these values affect data types throughout the design
MAX_NUM_SC = 64;
MAX_CP_LEN = 32;
MAX_NUM_SAMPS = 50e3;
MAX_NUM_SYMS = 600;
MAX_NUM_BYTES = 2^16-1;

%% Define the LTS correlation coefficients

%Call util scripts to generate the PHY preamble signals
PLCP_Preamble = PLCP_Preamble_gen;

% Rx PHY uses Fix3_0 to store coefficients
longCorr_coef_nbits = 3;
longCorr_coef_bp = 0;
long_cor_acc_n_bits = 6 * 2;

%Scale, conjugate and time-reverse the standard LTS
longCorr_coef = fliplr(conj(PLCP_Preamble.LTS_t./max(abs(PLCP_Preamble.LTS_t))));

longCorr_coef_i = [3*real(longCorr_coef)];
longCorr_coef_q = [3*imag(longCorr_coef)];


%Define the size of the circular sample buffer before the FFT
% This buffer must be large enough to hold the full LTS section of the
% preamble plus enough post-preamble samples to acommodate the latency
% of the FFT taking the transforms of the LTS. 4*lengh(LTS) is enough
% for the standard preamble and 64 subcarriers
preFFT_sampBuff_numSamps = 4*MAX_NUM_SC;

%Define the frequency-domain training symbol coefficients
% sign() here stores +/-1 (LTS is BPSK in freq domain) for smaller memory in  hardware

%L-LTF (legacy training symbol - 52 non-zero subcarriers)
l_ltf_f = sign(PLCP_Preamble.LTS_f);

%HT-LTF (11n training symbol - 56 non-zero subcarriers)
ht_ltf_f = l_ltf_f;
ht_ltf_f([28 29 37 38]) = [-1 -1 +1 +1];


%Initialize a vector defining the subcarrier map
% This vector is used by the interleaver control logic to select which
% subcarriers carry data symbols. A value of MAX_NUM_SC tells the hardware to
% not use the subcarrier for data.
sc_ind_data_11a = [2:7 9:21 23:27 39:43 45:57 59:64];
sc_data_sym_map_11a = MAX_NUM_SC*ones(1,MAX_NUM_SC);
sc_data_sym_map_11a(sc_ind_data_11a) = fftshift(0:length(sc_ind_data_11a)-1);
sc_data_sym_map_11a_bool = double(sc_data_sym_map_11a ~= MAX_NUM_SC);

sc_ind_data_11n = [2:7 9:21 23:29 37:43 45:57 59:64];
sc_data_sym_map_11n = MAX_NUM_SC*ones(1,MAX_NUM_SC);
sc_data_sym_map_11n(sc_ind_data_11n) = fftshift(0:length(sc_ind_data_11n)-1);
sc_data_sym_map_11n_bool = double(sc_data_sym_map_11n ~= MAX_NUM_SC);

%%
%Initial values for Rx PHY registers
PHY_CONFIG_NUM_SC = 64;
PHY_CONFIG_CP_LEN = 16;
PHY_CONFIG_FFT_SCALING = bin2dec('000101');

% FFT offset depends on PHY samp rate (and waveform format?)
%  20: FFT_OFFSET=2 for zero cyclic prefix samps
%  40: FFT_OFFSET=4 for zero cyclic prefix samps
%PHY_CONFIG_FFT_OFFSET = (1+2*(rx_sim.samp_rate==40)) + 0;
PHY_CONFIG_FFT_OFFSET = 7;

PHY_CONFIG_RSSI_SUM_LEN = 8;

PHY_SIGNAL_MIN_LEN = 14;

PHY_CONFIG_LTS_CORR_THRESH_LOWSNR = 12500;
PHY_CONFIG_LTS_CORR_THRESH_HIGHSNR = 12500;
PHY_CONFIG_LTS_CORR_PEAKTYPE_THRESH_LOWSNR = 12500;
PHY_CONFIG_LTS_CORR_PEAKTYPE_THRESH_HIGHSNR = 12500;
PHY_CONFIG_LTS_CORR_RSSI_THRESH = PHY_CONFIG_RSSI_SUM_LEN*400;

PHY_CONFIG_LTS_CORR_TIMEOUT = 175;

%Good defaults for hw
PHY_CONFIG_PKT_DET_CORR_THRESH_DSSS = 0.85*2^6;%1.5 * 2^6;%hex2dec('FF');%(1) * 2^7;
PHY_CONFIG_PKT_DET_ENERGY_THRESH_DSSS = 0;%400;%hex2dec('3FF');%(20) * 2^4; %UFix10_0

%For sim testing with signal captured post-AGC
%PHY_CONFIG_PKT_DET_CORR_THRESH_DSSS = 1.0 * 2^6;%hex2dec('FF');%(1) * 2^7;
%PHY_CONFIG_PKT_DET_ENERGY_THRESH_DSSS = 0;%hex2dec('3FF');%(20) * 2^4; %UFix10_0

% Channel estimate smooothing
%PHY_CONFIG_H_EST_SMOOTHING_A = 2867; %round(0.7 * 2^12);
%PHY_CONFIG_H_EST_SMOOTHING_B = 614; %round(0.15 * 2^12);
PHY_CONFIG_H_EST_SMOOTHING_A = 2^12-1;
PHY_CONFIG_H_EST_SMOOTHING_B = 0;

%h = [.17 .7 .17 zeros(1,61)]; %Gives 0dB gain at 38 (FFT offset 5), better than 0dB gain at 32 (FFT offset 0)

%PHY_CONFIG_H_EST_SMOOTHING_A = 2000;
%PHY_CONFIG_H_EST_SMOOTHING_B = 0;

PHY_CONFIG_PKT_DET_CORR_THRESH = 200;%(0.75) * 2^8; %UFix8_8 threshold - CLK2X
PHY_CONFIG_PKT_DET_ENERGY_THRESH = 1;%1; %UFix14_4 thresh; set to low non-zero value %CLK2X - changed from 1
PHY_CONFIG_PKT_DET_MIN_DURR = 15; %UFix4_0 duration
PHY_CONFIG_PKT_DET_RESET_EXT_DUR = hex2dec('3F');

CS_CONFIG_CS_RSSI_THRESH = 300 * PHY_CONFIG_RSSI_SUM_LEN;
CS_CONFIG_POSTRX_EXTENSION = 6*20; %6usec as 120 20MHz samples

SOFT_DEMAP_SCALE_BPSK = 15;
SOFT_DEMAP_SCALE_QPSK = 15;
SOFT_DEMAP_SCALE_16QAM = 18;
SOFT_DEMAP_SCALE_64QAM = 22;

REG_RX_PktDet_AutoCorr_Config = ...
    2^0  *  (PHY_CONFIG_PKT_DET_CORR_THRESH) +...%b[7:0] UFix8_8
    2^8  *  (PHY_CONFIG_PKT_DET_ENERGY_THRESH) +...%b[21:8] UFix14_8
    2^22 *  (PHY_CONFIG_PKT_DET_MIN_DURR) +...%b[25:22]
    2^26 *  (PHY_CONFIG_PKT_DET_RESET_EXT_DUR) + ...%b[31:26]
    0;

REG_RX_LTS_Corr_Thresh = ...
    2^0  *  (PHY_CONFIG_LTS_CORR_THRESH_LOWSNR) +... %b[15:0]
    2^16  * (PHY_CONFIG_LTS_CORR_THRESH_HIGHSNR) +... %b[31:16]
    0;

REG_RX_LTS_Corr_PeakType_Thresh = ...
    2^0  *  (PHY_CONFIG_LTS_CORR_PEAKTYPE_THRESH_LOWSNR) +... %b[15:0]
    2^16  * (PHY_CONFIG_LTS_CORR_PEAKTYPE_THRESH_HIGHSNR) +... %b[31:16]
    0;

REG_RX_Chan_Est_Smoothing = ...
    2^0  *  (PHY_CONFIG_H_EST_SMOOTHING_A) +... %b[11:0]
    2^12  * (PHY_CONFIG_H_EST_SMOOTHING_B) +... %b[23:12]
    2^24  * 18; %PHY mode det threshold
    0;

REG_RX_LTS_Corr_Confg = ...
    2^0 *  (PHY_CONFIG_LTS_CORR_TIMEOUT) + ... %b[7:0]
    2^8 *  (PHY_CONFIG_LTS_CORR_RSSI_THRESH) + ... %b[23:8]
    2^24 * 7 + ... %b[26:24] - 0x1/2/4 enables 63/64/65-sample spacing for LTS corr
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
    2^3  * 1 + ... %Swap order of chan est u32 writes
    2^4  * 1 + ... %Block DSSS Rx until DSSS pkt det has occurred
    2^5  * 0 + ... %Bypass CFO est/correction
    2^6  * 1 + ... %Enable chan est recording to pkt buf
    2^7  * 0 + ... %Enable switching diversity
    2^8  * 0 + ... %x 
    2^9  * 1 + ... %Enable pkt det on Ant A
    2^10 * 0 + ... %Enable pkt det on Ant B
    2^11 * 0 + ... %Enable pkt det on Ant C
    2^12 * 0 + ... %Enable pkt det on Ant D
    2^13 * 0 + ... %Enable ext pkt det
    2^14 * 0 + ... %PHY CCA mode (0=any, 1=all)
    2^15 * 0 + ... %Manual ant sel when sel div disabled (2-bits, 00=RFA)
    2^17 * 2 + ... %Max SIGNAL.LENGTH value, in kB (UFix4_0)
    2^21 * 0 + ... %Require auto-corr and RSSI pkt det for OFDM Rx
    2^22 * 0 + ... %Rate-Length Busy holds pkt det high
    2^23 * 1 + ... %DSSS asserts CCA busy
    2^24 * 1 + ... %Enable 11n Rx support
    2^25 * 1 + ... %Enable detection of 11ac waveforms based on VHT-SIG modulation
    2^26 * 0 + ... %Require RSSI and AutoCorr detection for DSSS Rx
    0;

REG_RX_DSSS_RX_CONFIG = ...
    2^0  * (48) + ... %b[7:0]: UFix8_0 SYNC matching score thresh
    2^8  * (45) + ... %b[15:8]: UFix8_0 SYNC matching timeout (multiplied by 32 in hw)
    2^16 * (52) + ... %b[23:16]: UFix8_0 SFD matching timeout (multiplied by 32 in hw)
    2^24 * (39) + ... %b[31:24]: UFix8_0 SYNC search time (samples)
    0;

REG_RX_PktDet_DSSS_Config = ...
    2^0  *  (PHY_CONFIG_PKT_DET_CORR_THRESH_DSSS) +...  %b[7:0] UFix8_7
    2^8  *  (PHY_CONFIG_PKT_DET_ENERGY_THRESH_DSSS) +...%b[17:8] UFix10_4
    0;

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
    2^16 * 35 + ... %b[23:16]: Pkt buf offset for Rx bytes (u64 words)
    2^24 * 3 + ... %b[31:24]: Pkt buf offset for chan est (u64 words)
    0;

REG_RX_FEC_Config = ...
    2^0  * (SOFT_DEMAP_SCALE_BPSK) + ...
    2^5  * (SOFT_DEMAP_SCALE_QPSK) + ...
    2^10  * (SOFT_DEMAP_SCALE_16QAM) + ...
    2^15 * (SOFT_DEMAP_SCALE_64QAM) + ...
    0;

REG_RX_PKT_BUF_Max_Write_Addr = 3800;

%%
bit_scrambler_lfsr = ones(1,7);
bit_scrambler_lfsr_states = zeros(127, 7);
scr = zeros(1,127);
for ii=1:127
    bit_scrambler_lfsr_states(ii, :) = bit_scrambler_lfsr;

    %LFSR polynomial: x^7 + x^4 + 1
    x = xor(bit_scrambler_lfsr(4), bit_scrambler_lfsr(7));
    bit_scrambler_lfsr = [x bit_scrambler_lfsr(1:6)];

    scr(ii) = x;
end

%Convert bitwise descrambler states to bytewise descramber states
% same as bit_scrambler_lfsr_bytes = bi2de(reshape(repmat(scr, 1, 8), 8, 127)', 'right-msb');
%  without using bi2de
bit_scrambler_lfsr_bytes = (reshape(repmat(scr, 1, 8), 8, 127)');
bit_scrambler_lfsr_bytes = sum(bit_scrambler_lfsr_bytes .* repmat(2.^[0:7], 127, 1),2);
 
%%
%Generate the vector of addresses for the bytewise descramber ROM
scr = [scr scr(1:10)];
scr_ind_rev = zeros(1,128);
for ii=1:127
    %Same as scr_ind_rev(1 + bi2de(scr(ii:ii+6))) = ii - 1;
    % without using bi2de
    scr_ind_rev(1 + sum(2.^[0:1:6].*scr(ii:ii+6))) = ii - 1;
end
clear scr x bit_scrambler_lfsr ii

%% Cyclic Redundancy Check parameters
CRCPolynomial32 = hex2dec('04c11db7'); %CRC-32
CRC_Table32 = CRC_table_gen(CRCPolynomial32, 32);

CRCPolynomial8 = hex2dec('07'); %CRC-8
CRC_Table8 = CRC_table_gen(CRCPolynomial8, 8);

%%
[mcs_rom_11ag, mcs_rom_11n] = mcs_info_rom_init();

%% Define numbers of coded and data bits per MCS
NCBPS_11a_BPSK = 48;
NCBPS_11a_QPSK = 96;
NCBPS_11a_16QAM = 192;
NCBPS_11a_64QAM = 288;

NCBPS_11n_BPSK = 52;
NCBPS_11n_QPSK = 104;
NCBPS_11n_16QAM = 208;
NCBPS_11n_64QAM = 312;

%Max bits-per-symbol determines size of de-interleaver RAM
MAX_NCBPS = NCBPS_11n_64QAM;

NDBPS_11a_BPSK12 = 24;
NDBPS_11a_BPSK34 = 36;
NDBPS_11a_QPSK12 = 48;
NDBPS_11a_QPSK34 = 72;
NDBPS_11a_16QAM12 = 96;
NDBPS_11a_16QAM34 = 144;
NDBPS_11a_64QAM23 = 192;
NDBPS_11a_64QAM34 = 216;

NDBPS_11n_BPSK12 = 26;
NDBPS_11n_QPSK12 = 52;
NDBPS_11n_QPSK34 = 78;
NDBPS_11n_16QAM12 = 104;
NDBPS_11n_16QAM34 = 156;
NDBPS_11n_64QAM23 = 208;
NDBPS_11n_64QAM34 = 234;
NDBPS_11n_64QAM56 = 260;

%% Calculate de-interleaving vectors
%11a
% BPSK
N_CBPS = NCBPS_11a_BPSK;
N_BPSC = 1;
s = max(N_BPSC/2, 1);

%Interleaver (k=src bit index -> j=dest bit index)
k = 0:N_CBPS-1;
i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
%BPSK doesn't need j

interleave_11a_BPSK = i;
clear N_CBPS N_BPSC s k i

% QPSK
N_CBPS = NCBPS_11a_QPSK;
N_BPSC = 2;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(16*i/N_CBPS)), s);
interleave_11a_QPSK = j;
clear N_CBPS N_BPSC s k i j

% 16-QAM
N_CBPS = NCBPS_11a_16QAM;
N_BPSC = 4;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(16*i/N_CBPS)), s);
interleave_11a_16QAM = j;
clear N_CBPS N_BPSC s k i j

% 64-QAM
N_CBPS = NCBPS_11a_64QAM;
N_BPSC = 6;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (N_CBPS/16) .* mod(k,16) + floor(k/16);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(16*i/N_CBPS)), s);
interleave_11a_64QAM = j;
clear N_CBPS N_BPSC s k i j

%FFT Shift
interleave_11a_BPSK = mod(interleave_11a_BPSK + (NCBPS_11a_BPSK/2), NCBPS_11a_BPSK);
interleave_11a_QPSK = mod(interleave_11a_QPSK + (NCBPS_11a_QPSK/2), NCBPS_11a_QPSK);
interleave_11a_16QAM = mod(interleave_11a_16QAM + (NCBPS_11a_16QAM/2), NCBPS_11a_16QAM);
interleave_11a_64QAM = mod(interleave_11a_64QAM + (NCBPS_11a_64QAM/2), NCBPS_11a_64QAM);

%%
%11n
% BPSK
N_CBPS = NCBPS_11n_BPSK;
N_BPSC = 1;
s = max(N_BPSC/2, 1);

%Interleaver (k=src bit index -> j=dest bit index)
k = 0:N_CBPS-1;
i = (4 * N_BPSC) .* mod(k,13) + floor(k/13);
%BPSK doesn't need j

interleave_11n_BPSK = i;
clear N_CBPS N_BPSC s k i

% QPSK
N_CBPS = NCBPS_11n_QPSK;
N_BPSC = 2;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (4 * N_BPSC) .* mod(k,13) + floor(k/13);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(13*i/N_CBPS)), s);
interleave_11n_QPSK = j;
clear N_CBPS N_BPSC s k i j

% 16-QAM
N_CBPS = NCBPS_11n_16QAM;
N_BPSC = 4;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (4 * N_BPSC) .* mod(k,13) + floor(k/13);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(13*i/N_CBPS)), s);
interleave_11n_16QAM = j;
clear N_CBPS N_BPSC s k i j

% 64-QAM
N_CBPS = NCBPS_11n_64QAM;
N_BPSC = 6;
s = max(N_BPSC/2, 1);

k = 0:N_CBPS-1;
i = (4 * N_BPSC) .* mod(k,13) + floor(k/13);
j = s * floor(i/s) + mod( (i + N_CBPS - floor(13*i/N_CBPS)), s);
interleave_11n_64QAM = j;
clear N_CBPS N_BPSC s k i j

%FFT Shift
interleave_11n_BPSK = mod(interleave_11n_BPSK + (NCBPS_11n_BPSK/2), NCBPS_11n_BPSK);
interleave_11n_QPSK = mod(interleave_11n_QPSK + (NCBPS_11n_QPSK/2), NCBPS_11n_QPSK);
interleave_11n_16QAM = mod(interleave_11n_16QAM + (NCBPS_11n_16QAM/2), NCBPS_11n_16QAM);
interleave_11n_64QAM = mod(interleave_11n_64QAM + (NCBPS_11n_64QAM/2), NCBPS_11n_64QAM);


%De-interleaver ROM contents - one ROM used for all rates
deinterleave_ROM = [];
deinterleave_ROM = [deinterleave_ROM interleave_11a_BPSK zeros(1, 512-length(interleave_11a_BPSK))];
deinterleave_ROM = [deinterleave_ROM interleave_11a_QPSK zeros(1, 512-length(interleave_11a_QPSK))];
deinterleave_ROM = [deinterleave_ROM interleave_11a_16QAM zeros(1, 512-length(interleave_11a_16QAM))];
deinterleave_ROM = [deinterleave_ROM interleave_11a_64QAM zeros(1, 512-length(interleave_11a_64QAM))];

deinterleave_ROM = [deinterleave_ROM interleave_11n_BPSK zeros(1, 512-length(interleave_11n_BPSK))];
deinterleave_ROM = [deinterleave_ROM interleave_11n_QPSK zeros(1, 512-length(interleave_11n_QPSK))];
deinterleave_ROM = [deinterleave_ROM interleave_11n_16QAM zeros(1, 512-length(interleave_11n_16QAM))];
deinterleave_ROM = [deinterleave_ROM interleave_11n_64QAM zeros(1, 512-length(interleave_11n_64QAM))];


%%%%%%%%% NEW DSSS INIT
% Define resampled spreading sequences
%  loads variables spread_seq_<11,20,40>
load('./util/util_resampled_spread_seq.mat');

% Extract and scale 20-sample spreading sequence
spread_seq_20 = spread_seq_20(40+(0:19));
spread_seq_20 = spread_seq_20 / sum(spread_seq_20); % for G=1 fitler response
spread_seq_20 = spread_seq_20 - mean(spread_seq_20); % for DC=0 response

DSSS_CLKS_PER_SAMP = 8; %160MHz clk / 20MSps I/Q
DSSS_SAMPS_PER_SYM = 20; %sampling frequency, effective spreading seq length
DSSS_BIT_MATCH_RAM_WIDTH = 32; % width of word-access ports
DSSS_NUM_SYMS_PER_OFFSET = (2 * DSSS_CLKS_PER_SAMP * DSSS_BIT_MATCH_RAM_WIDTH);
DSSS_NUM_SYMS_IN_RAM = DSSS_SAMPS_PER_SYM * DSSS_NUM_SYMS_PER_OFFSET;

% Pad the target bit and mask vectors - the padding must be first so the matching
%  logic ignores the oldest bits (post-padding would ignore new bits)
% Clause 16 scrambled SYNC:
clause16_scr_sync = [0,1,1,1,1,1,1,0,1,1,1,0,1,1,0,0,1,1,1,0,1,0,0,0,1,0,1,0,0,1,0,0,1,1,1,1,1,0,0,1,1,0,0,1,0,1,0,1,1,0,0,0,1,1,0,0,0,0,1,0,0,1,0,1,1,1,1,0,1,0,1,0,1,0,0,0,0,0,1,0,1,1,0,1,0,1,1,1,0,0,1,0,0,0,1,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,1,0,0,0,0,1,1,0,1,0,0,1,1,0,1,1,0];

target_bits = clause16_scr_sync(5:64);
%mask1 = ones(1, length(target_bits));
mask1 = [zeros(1,12) ones(1, 48)];

target_bits_pad = [zeros(1, DSSS_NUM_SYMS_PER_OFFSET - length(target_bits)) target_bits ];
mask1_pad = [zeros(1, DSSS_NUM_SYMS_PER_OFFSET - length(mask1)) mask1];

% Target bits RAM must contain 32 copies (BIT_MATCH_RAM_WIDTH) of the target
%  bit sequence. Each copy is cyclically shifted from the previous by 1
target_bits_ram_init = zeros(1, DSSS_NUM_SYMS_IN_RAM/32);
mask_1_ram_init = zeros(1, DSSS_NUM_SYMS_IN_RAM/32);

f_bit_vec_to_words = @(bv) 2.^(0:31) * reshape(bv(:), 32, []);

for ii = 0:31
    x = circshift(target_bits_pad, [0 -31+ii]);
    x = f_bit_vec_to_words(x);
    target_bits_ram_init([1:2*DSSS_CLKS_PER_SAMP] + ii*2*DSSS_CLKS_PER_SAMP) = x;

    x = circshift(mask1_pad, [0 -31+ii]);
    x = f_bit_vec_to_words(x);
    mask_1_ram_init([1:2*DSSS_CLKS_PER_SAMP] + ii*2*DSSS_CLKS_PER_SAMP) = x;

end

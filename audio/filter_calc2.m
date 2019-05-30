% TPA response plotter
% march 2015, gmarsh

% output filter (1D14A-150M)
c = 1.5e-6;
l = 10e-6;
rl = 13e-3;

% speaker (Alpair 10M)
% motional part of electrical model left out here, as it's largely insignificant here
re = 4.6;
le = 38e-6;

% speaker zobel network
rz = 7;
cz = 1.5e-6;

% == boring math below ==

% make plotting vector
% replace 0hz with 1Hz to avoid divide by zero
fplot = 100000;
f = linspace(fplot/1000,fplot,1000);
w = f*2*pi;

% calculate speaker impedance
zspeaker = re + j*w*le;
zzobel = rz + 1./(j*w*cz);
zload = 1./(1./zspeaker + 1./zzobel);

% divide speaker load by two (since this is a BTL amplifier)
zload = zload ./ 2;

zc = 1./(j*w*c);
zshunt = 1./(1./zload + 1./zc);
zl = rl + j*w*l;
response = zshunt./(zshunt + zl);

% plot result
subplot(2,1,1);
semilogx(f,20*log10(abs(response)));
ylabel('dB');
title('Electrical response');
grid on;
subplot(2,1,2);
semilogx(f,abs(zload*2));
ylabel('ohms');
title('Amp load');
grid on;
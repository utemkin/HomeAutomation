rload = 2; % half of actual load in BTL
c = 1.5e-6;
l = 10e-6;
rl = 13e-3;

fplot = 50000;
%f = linspace(fplot/1000,fplot,1000);
f = [10000 16000 18000 20000 22000 30000 40000 400000 600000];

% calc filter characteristics
wo = 1./sqrt(l.*c);
q = wo*rload*c; % q doesn't include Rload (figure out later)
fo = wo/(2*pi);
disp(q)
disp(fo)

zload = rload;
zc = -j./(2*pi*f*c);
zl = j*2*pi*f*l + rl;
zrc = 1./(1./zc + 1./zload);
resp = zrc ./ (zrc + zl);
logresp = 20*log10(abs(resp));
disp(logresp);

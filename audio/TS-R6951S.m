f = logspace(1, 5, 1000);
w = f * 2 * pi;

impBass = 4 + j * w * 0.53e-3 + 1 ./ (1 ./ 13 + 1 ./ (j * w * 5.6e-3) + j * w * 700e-6)

impTweeter = 4.6 + j * w * 38e-6 + 1 ./ (j * w * 5.3e-6);

imp = 1 ./ (1 ./ impBass + 1 ./ impTweeter)

subplot(2,1,1);
  semilogx(f, abs(imp));
  xlabel('f, Hz');
  ylabel('|Imp|, Ohm');
  grid on;

subplot(2,1,2);
  semilogx(f, angle(imp) ./pi * 180);
  xlabel('f, Hz');
  ylabel('Phase, deg');
  grid on;

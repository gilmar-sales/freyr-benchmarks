
- Query metricas
```sql
WITH update_slices AS (SELECT dur, ROW_NUMBER() OVER (ORDER BY dur) AS rn, COUNT(*) OVER ()                  AS total
                       FROM slice
                       WHERE name = 'Update' AND dur > 0),
     stats AS (SELECT COUNT(*) AS cnt, SUM(dur) AS sum_dur, AVG(dur) AS avg_dur, SUM(dur * dur) AS sum_sq
               FROM update_slices),
     median_calc AS (SELECT AVG(dur) AS median_dur FROM update_slices WHERE rn IN ((total + 1) / 2, (total + 2) / 2))
SELECT s.cnt                                                          AS execucoes,
       ROUND(s.sum_dur / 1e6, 3)                                      AS total_acumulado_ms,
       ROUND(s.avg_dur / 1e6, 3)                                      AS media_ms,
       ROUND(m.median_dur / 1e6, 3)                                   AS mediana_ms,
       ROUND(SQRT(s.sum_sq / s.cnt - s.avg_dur * s.avg_dur) / 1e6, 3) AS desvio_padrao_ms
FROM stats s,
     median_calc m;
```

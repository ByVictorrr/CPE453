import jdk.internal.org.objectweb.asm.commons.StaticInitMerger;
import sun.awt.image.ImageWatched;

import java.util.*;
import java.util.stream.Collectors;

public class Main {
    public static void main(String [] args) {

        Queue<Process> ps = new LinkedList<>();
        ps.add(new Process("A", 10));
        ps.add(new Process("B", 3));
        ps.add(new Process("C", 4));
        ps.add(new Process("D", 7));
        ps.add(new Process("E", 6));
        ps.forEach(p->{
            System.out.println(p.label + p.time);
        });
        Map<String, Integer> m_turn_time = rr_turn_around(ps);


    }
    private static Map<String, Integer> rr_turn_around(Queue<Process> ps) {
        Map<String, Integer> m_turn_time = (Map<String, Integer>) get_init_map(ps);
        Process p;
        int time = 0;
        while ((p = ps.poll()) != null) {
            time++;
            m_turn_time.put(p.label, time);
            p.time--;
            if (p.time  > 0)
                ps.add(p);
        }
        return m_turn_time;
    }
    private static Map<String, Float> rr_turn_around(Queue<Process> ps, Process IO) {
        Map<String, Float> m_turn_time = (Map<String, Float>) get_init_map(ps);
        Process p;
        float time = 0; // in s
        int i=0;
        // alternate from .5 to adding 1 while IO.time is greater than 0
        // after just alternate a quantum of 1s
        while ((p = ps.poll()) != null) {
            if(i%2==0){
                time+=.5;
            }else{
                time++;
            }
            m_turn_time.put(p.label, time);
            p.time--;
            if (p.time  > 0)
                ps.add(p);
        }
        return m_turn_time;
    }


    private static Map<String, ?> get_init_map(Queue<Process>ps){
        HashMap<String,Double> m_turn_time = new HashMap<>();
        List<Process> ps_l = ps.stream().collect(Collectors.toList());
        for (int i = 0; i < ps_l.size(); i++){
            m_turn_time.put(ps_l.get(i).label, 0.0);
        }
        return m_turn_time;
    }
    static class Process{
       private String label ;
       private Integer time;
       public  Process(String _label, Integer _time){
           label=_label;
           time = _time;
       }
    }

}

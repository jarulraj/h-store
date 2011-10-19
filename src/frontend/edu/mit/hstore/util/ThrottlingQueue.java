package edu.mit.hstore.util;

import java.util.Collection;
import java.util.Iterator;
import java.util.Queue;

import edu.brown.utils.ProfileMeasurement;

public class ThrottlingQueue<E> implements Queue<E> {

    private final Queue<E> queue;
    
    private boolean throttled;
    private int queue_max;
    private int queue_release;
    private double queue_release_factor;
    private final int queue_increase;
    private final ProfileMeasurement throttle_time;
            
    public ThrottlingQueue(Queue<E> queue, int queue_max, double queue_release, int queue_increase) {
        this.queue = queue;
        this.throttled = false;
        this.queue_max = queue_max;
        this.queue_increase = queue_increase;
        this.queue_release_factor = queue_release;
        this.queue_release = Math.max((int)(this.queue_max * this.queue_release_factor), 1);
        this.throttle_time = new ProfileMeasurement("throttling");
//        if (hstore_site.getHStoreConf().site.status_show_executor_info) {
//            this.throttle_time.resetOnEvent(hstore_site.getStartWorkloadObservable());
//        }
    }

    public ProfileMeasurement getThrottleTime() {
        return (this.throttle_time);
    }
    public Queue<E> getQueue() {
        return (this.queue);
    }
    public boolean isThrottled() {
        return (this.throttled);
    }
    public int getQueueMax() {
        return (this.queue_max);
    }
    public int getQueueRelease() {
        return (this.queue_release);
    }
    public int getQueueIncrease() {
        return (this.queue_increase);
    }
    
    /**
     * Check whether the size of this queue is greater than our max limit
     * We don't need to worry if this is 100% accurate, so we won't block here
     */
    public void checkThrottling() {
        this.checkThrottling(true);
    }
    
    public void checkThrottling(boolean increase) {
        int size = this.queue.size();
        if (size > this.queue_max) {
            this.throttled = true;
        } 
        else if (size > this.queue_release) {
            this.throttled = false;
        }
        else if (increase && this.throttled == false && size == 0) {
            this.queue_max += this.queue_increase;
            this.queue_release = Math.max((int)(this.queue_max * this.queue_release_factor), 1);
        }
    }
    
    @Override
    public boolean add(E e) {
        return (this.offer(e));
    }
    @Override
    public boolean offer(E e) {
        return this.offer(e, false);
    }
    /**
     * Offer an element to the queue. If force is true,
     * then it will always be inserted and not count against
     * the throttling counter.
     * @param e
     * @param force
     * @return
     */
    public boolean offer(E e, boolean force) {
        if (force) {
            this.queue.offer(e);
            return (true);
        } else if (this.throttled) {
            return (false);
        }
        
        boolean ret = this.queue.offer(e);
        if (ret) this.checkThrottling();
        return (ret);
    }
    @Override
    public boolean remove(Object o) {
        boolean ret = this.queue.remove(o);
        if (ret) this.checkThrottling();
        return (ret);
    }
    @Override
    public E poll() {
        E e = this.queue.poll();
        if (e != null) this.checkThrottling();
        return (e);
    }
    @Override
    public E remove() {
        E e = this.queue.remove();
        if (e != null) this.checkThrottling();
        return (e);
    }
    
    @Override
    public void clear() {
        if (this.throttled) ProfileMeasurement.stop(false, this.throttle_time);
        this.throttled = false;
        this.queue.clear();
    }
    @Override
    public boolean removeAll(Collection<?> c) {
        return (this.queue.removeAll(c));
    }
    @Override
    public boolean retainAll(Collection<?> c) {
        return (this.queue.retainAll(c));
    }
    @Override
    public boolean addAll(Collection<? extends E> c) {
        return (this.queue.addAll(c));
    }
    @Override
    public E element() {
        return (this.queue.element());
    }
    @Override
    public E peek() {
        return (this.queue.peek());
    }
    @Override
    public boolean contains(Object o) {
        return (this.queue.contains(o));
    }
    @Override
    public boolean containsAll(Collection<?> c) {
        return (this.queue.containsAll(c));
    }
    @Override
    public boolean isEmpty() {
        return (this.queue.isEmpty());
    }
    @Override
    public Iterator<E> iterator() {
        return (this.queue.iterator());
    }
    @Override
    public int size() {
        return (this.queue.size());
    }
    @Override
    public Object[] toArray() {
        return (this.queue.toArray());
    }
    @Override
    public <T> T[] toArray(T[] a) {
        return (this.queue.toArray(a));
    }

    @Override
    public String toString() {
        return String.format("%s [max=%d / release=%d / increase=%d]",
                             this.queue.toString(),
                             this.queue_max, this.queue_release, this.queue_increase);
    }

    
    
}
